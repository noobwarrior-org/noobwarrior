/*
 * Copyright (C) 2026 Hattozo
 *
 * This file is part of noobWarrior.
 *
 * noobWarrior is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * noobWarrior is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with noobWarrior; if not, see
 * <https://www.gnu.org/licenses/>.
 */
// === noobWarrior ===
// File: Backup.cpp
// Started by: Hattozo
// Started on: 3/5/2025
// Description: All functions that concern backing up data from Roblox servers to your computer belong here
#include <NoobWarrior/Backup.h>
#include <NoobWarrior/Config.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Roblox/Api/Asset.h>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <cmath>
#include <iostream>
#include <filesystem>
#include <sstream>

using namespace NoobWarrior;
using json = nlohmann::json;

static const std::map<long, std::string> sHttpStatusMessages = {
    {100, "Continue"},
    {101, "Switching Protocols"},
    {200, "OK"},
    {201, "Created"},
    {202, "Accepted"},
    {204, "No Content"},
    {301, "Moved Permanently"},
    {302, "Found"},
    {304, "Not Modified"},
    {400, "Bad Request"},
    {401, "Unauthorized"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {500, "Internal Server Error"},
    {502, "Bad Gateway"},
    {503, "Service Unavailable"}
};

static int CountDigits(int c) {
    if (c == 0)
        return 1;
    return (int)floor(log10(abs(c))) + 1;
}

static long ConvertISO8601ToTimestamp(const std::string& iso8601_string) {
    std::tm t{};
    std::istringstream ss(iso8601_string);
    ss >> std::get_time(&t, "%Y-%m-%dT%H:%M:%S%z");
    if (ss.fail())
        return -1;
    std::time_t time_since_epoch = mktime(&t);
    if (time_since_epoch == -1)
        return -2;
    return static_cast<long>(time_since_epoch);
}

static size_t CurlWriteToFile(char* ptr, size_t size, size_t nmemb, FILE *userdata) {
    size_t written;
    written = fwrite(ptr, size, nmemb, userdata);
    return written;
}

static size_t CurlWriteToBuf(void *contents, size_t size, size_t nmemb, std::vector<char> *buffer) {
    size_t totalSize = size * nmemb;
    buffer->insert(buffer->end(), (char*)contents, (char*)contents + totalSize);
    return totalSize;
}

static size_t HeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata) {
    size_t totalSize = nitems * size;
    char* contentDepositionHeader = (char*)"Content-Disposition: ";
    size_t headerLength = strlen(contentDepositionHeader);

    if (totalSize > headerLength && strncmp(buffer, contentDepositionHeader, headerLength) == 0) {
        // our content deposition header.
        char* val = buffer + headerLength;
        userdata = val; // our userdata in this case is the file name, since the content deposition header contains that stuff.
    }

    return totalSize;
}

int Core::DownloadAssets(DownloadAssetArgs args) {
    if (args.OutStream == nullptr) args.OutStream = &std::cout;
    curl_version_info_data *vinfo = curl_version_info(CURLVERSION_NOW);
    if (!(vinfo->features & CURL_VERSION_SSL))
        OutEx(args.OutStream, "AssetRequest", "WARNING! SSL in curl library is not enabled. HTTPS links will be unsupported!", args.OutDir);
    if (!std::filesystem::is_directory(args.OutDir)) {
        OutEx(args.OutStream, "AssetRequest", "Failed to download assets: Directory \"{}\" doesn't exist", args.OutDir);
        return -1;
    }
    CURL *handle = curl_easy_init();
    if (!handle) {
        OutEx(args.OutStream, "AssetRequest", "Failed to download assets: Failed to create curl handle");
        return 0;
    }
    curl_easy_setopt(handle, CURLOPT_USERAGENT, "Roblox/WinINet"); // use the same user agent that the Roblox client uses.
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, &CurlWriteToFile);
    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, &HeaderCallback);
    for (int i = 0; i < args.Id.size(); i++) {
        OutEx(args.OutStream, "AssetRequest", "Downloading ID {}", args.Id.at(i));

        // by default we create a placeholder file with the ID as its name.
        int64_t id = args.Id.at(i);
        size_t idDigits = CountDigits(id);
        char* fileName = (char*)malloc(idDigits + 1);
        snprintf(fileName, idDigits + 1, "%i", (int)id);

        std::optional<std::string> download_url = GetConfig()->GetKeyValue<std::string>("internet.roblox.asset_download");
        if (!download_url.has_value())
            return -2;

        std::string fmtApiCall = std::vformat(download_url.value(), std::make_format_args(id));
        std::string fileDir = args.OutDir + "/" + fileName;

        FILE* filePointer = fopen(fileDir.c_str(), "wb");
        if (filePointer == NULL) {
            OutEx(args.OutStream, "AssetRequest", "Failed to download ID %i: Failed to create file");
            continue;
        }

        // we had this pointing to memory that was allocated to the number of digits in our asset id, so we could have the file name be the asset ID.
        // now we're done with that part, and we might soon rename it to something else anyways if the user decides to not like ID's as the file name.
        free(fileName);

        curl_easy_setopt(handle, CURLOPT_URL, fmtApiCall.c_str());
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, filePointer);
        if (args.FileNameStyle == AssetFileNameStyle::Raw) // header callback makes the file name pointer point to content deposition's header value.
            curl_easy_setopt(handle, CURLOPT_HEADERDATA, fileName);

        CURLcode ret = curl_easy_perform(handle);
        if (ret == CURLE_OK) {
            long res;
            curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &res);
            if (res != 200) { OutEx(args.OutStream, "AssetRequest", "Failed to download ID {}: {} {}", id, res, sHttpStatusMessages.count(res) ? sHttpStatusMessages.at(res) : "Unknown"); }
        }
        if (ret != CURLE_OK)
            OutEx(args.OutStream, "AssetRequest", "Failed to download ID {}: Curl error {}, {}", id, (int)ret, curl_easy_strerror(ret));
        else if (args.FileNameStyle != AssetFileNameStyle::AssetId) { // because the file is already named with its corresponding asset id, so it's pointless to rename it to the same thing.
            std::string newFileDir = args.OutDir + "/" + fileName;
            rename(fileDir.c_str(), newFileDir.c_str());
        }
        
        fclose(filePointer);
    }
    OutEx(args.OutStream, "AssetRequest", !args.Id.empty() ? "Finished iterating through all IDs." : "Stopping, nothing to download.");
    curl_easy_cleanup(handle);
    return 1;
}

int Core::GetAssetDetails(int64_t id, Roblox::AssetDetails *details) {
    int ret = 0;
    std::optional<std::string> details_url = GetConfig()->GetKeyValue<std::string>("internet.roblox.asset_details");
    if (!details_url.has_value())
        return -2;

    std::vector<char> buffer;
    CURL *handle = curl_easy_init();
    if (!handle)
        return ret;
    curl_easy_setopt(handle, CURLOPT_USERAGENT, "Roblox/WinINet");
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, &CurlWriteToBuf);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(handle, CURLOPT_URL, std::vformat(details_url.value(), std::make_format_args(id)).c_str());
    CURLcode res = curl_easy_perform(handle);
    if (res == CURLE_OK) {
        json data = json::parse(buffer, nullptr, false);

        if (!data["errors"].is_null()) {
            ret = -1;
            goto cleanup;
        }

        details->TargetId = data["TargetId"];
        details->ProductType = !data["ProductType"].is_null() ? (data["ProductType"] != "Collectible Item" ? Roblox::ProductType::UserProduct : Roblox::ProductType::CollectibleItem) : Roblox::ProductType::None;
        details->AssetId = data["AssetId"];
        details->ProductId = data["ProductId"];
        details->Name = data["Name"];
        details->Description = data["Description"];
        details->AssetType = data["AssetTypeId"];
        details->CreatorId = data["Creator"]["Id"];
        details->CreatorName = data["Creator"]["Name"];
        details->CreatorType = data["Creator"]["CreatorType"] != "Group" ? Roblox::CreatorType::User : Roblox::CreatorType::Group;
        details->CreatorTargetId = data["Creator"]["CreatorTargetId"];
        details->CreatorHasVerifiedBadge = data["Creator"]["HasVerifiedBadge"];
        details->IconImageAssetId = data["IconImageAssetId"];
        details->Created = ConvertISO8601ToTimestamp(data["Created"]);
        details->Updated = ConvertISO8601ToTimestamp(data["Updated"]);
        details->PriceInRobux = !data["PriceInRobux"].is_null() ? static_cast<int>(data["PriceInRobux"]) : -1;
        details->PriceInTickets = !data["PriceInRobux"].is_null() ? static_cast<int>(data["PriceInTickets"]) : -1;
        details->Sales = data["Sales"];
        details->IsNew = data["IsNew"];
        details->IsForSale = data["IsForSale"];
        details->IsPublicDomain = data["IsPublicDomain"];
        details->IsLimited = data["IsLimited"];
        details->IsLimitedUnique = data["IsLimitedUnique"];
        details->MinimumMembershipLevel = data["MinimumMembershipLevel"];
        ret = 1;
    }
cleanup:
    curl_easy_cleanup(handle);
    return ret;
}

static void PopulateItemDescriptors() {
    
}

Backup::ItemDescriptor* Backup::ItemDescriptor_New() {
    ItemDescriptor *desc = new ItemDescriptor();
    desc->Children = (ItemDescriptor**) malloc(sizeof(uintptr_t));
    return desc;
}

void Backup::ItemDescriptor_Destroy(ItemDescriptor* desc) {
    for (int i = 0; i < desc->ChildrenSize; i++) {
        ItemDescriptor* child = desc->Children[i];
        ItemDescriptor_Destroy(child);
    }
    delete desc;
}

void Backup::ItemDescriptor_AddChild(ItemDescriptor* parent, ItemDescriptor* child) {
    if (child->Parent == parent) // same parent, useless
        return;

    if (child->Parent != nullptr)
        ItemDescriptor_RemoveChild(child->Parent, child);

    parent->ChildrenSize++;
    realloc(parent->Children, sizeof(uintptr_t) * parent->ChildrenSize);
    parent->Children[parent->ChildrenSize - 1] = child;
    child->Parent = parent;
}

void Backup::ItemDescriptor_RemoveChild(ItemDescriptor *parent, ItemDescriptor *child) {
    if (child->Parent != parent)
        return;

    parent->ChildrenSize--;
    realloc(parent->Children, sizeof(uintptr_t) * parent->ChildrenSize);
    child->Parent = nullptr;
}

Backup::ItemDescriptor** Backup::ItemDescriptor_GetChildren(ItemDescriptor* parent, int* size) {
    if (size != nullptr)
        *size = parent->ChildrenSize;
    return parent->Children;
}

Backup::Process* Backup::AllocateProcess(Core* core, ProcessOptions options) {
    auto optsMem = new ProcessOptions(options);
    auto proc = new Process();
    proc->Core = core;
    proc->Options = optsMem;

    ItemDescriptor* root = ItemDescriptor_New();
    proc->Root = root;

    return proc;
}

void Backup::DestroyProcess(Process* proc) {
    if (proc->Root != nullptr) {
        ItemDescriptor_Destroy(proc->Root);
        proc->Root = nullptr;
    }
    
    if (proc->Options != nullptr) {
        NOOBWARRIOR_FREE_PTR(proc->Options)
    }

    NOOBWARRIOR_FREE_PTR(proc)
}

Backup::Response Backup::StartProcess(Process* proc) {
    std::optional<std::string> asset_download_url = proc->Core->GetConfig()->GetKeyValue<std::string>("internet.roblox.asset_download");

    if (!asset_download_url.has_value())
        return Backup::Response::UrlNotSet;

    NetClient client(proc->Core->GetRobloxAuth()->GetActiveAccount());
    if (client.Fail())
        return Backup::Response::Failed;
    client.OnWriteToMemoryFinished([](std::vector<unsigned char> &data) {
        
    });
    client.Request(asset_download_url.value());
    return Backup::Response::Ok;
}