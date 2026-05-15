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
#include <NoobWarrior/Registry.h>
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

        std::optional<std::string> download_url = GetRegistry()->GetKeyValue<std::string>("internet.roblox.asset_download");
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
    std::optional<std::string> details_url = GetRegistry()->GetKeyValue<std::string>("internet.roblox.asset_details");
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

static HttpRequest CreateRbxReq(Core* core) {
    HttpRequest req;
    if (auto *acc = core->GetRbxKeychain()->GetActiveAccount()) {
        req.Cookie = ".ROBLOSECURITY=" + acc->Token + ";";
    }
    req.UserAgent = "Roblox/WinINet";
    return req;
}

Backup::ItemDescriptor::ItemDescriptor() {

}

Backup::ItemDescriptor::~ItemDescriptor() {
    for (int i = 0; i < this->Children.size(); i++) {
        ItemDescriptor* child = this->Children[i];
        delete child;
    }
}

Backup::ItemDescriptor* Backup::ItemDescriptor::GetParent() const {
    return this->Parent;
}

const std::vector<Backup::ItemDescriptor*>& Backup::ItemDescriptor::GetChildren() const {
    return this->Children;
}

void Backup::ItemDescriptor::AddChild(ItemDescriptor* child) {
    if (child->Parent == this) // same parent, useless
        return;

    if (child->Parent != nullptr)
        child->Parent->RemoveChild(child);

    this->Children.emplace_back(child);
    child->Parent = this;
}

void Backup::ItemDescriptor::RemoveChild(ItemDescriptor *child) {
    if (child->Parent != this)
        return;

    auto it = std::find(this->Children.begin(), this->Children.end(), child);
    if (it != this->Children.end()) {
        this->Children.erase(it);
        child->Parent = nullptr;
    }
}

Backup::Process::Process(Core* core, const ProcessOptions options) {
    mCore = core;
    mOptions = options;

    ItemDescriptor* root = new ItemDescriptor();
    mRoot = root;
    // Give it something to start off
    mRoot->Type = options.TargetItemType;
    mRoot->Id = options.TargetId;
    mRoot->Version = 0;
}

Backup::Process::~Process() {
    if (mRoot != nullptr) {
        NOOBWARRIOR_FREE_PTR(mRoot)
    }
}

Backup::Response Backup::Process::Start() {
    mOptions.Callback(Backup::State::Started, "Started backup process", 0);
    PopulateItemDescriptor(mRoot); // Phase 1: Populate the Item Descriptor with more Item Descriptors!!!
    DownloadItemDescriptorRecursively(mRoot); // Phase 2: Download them all!!!!!
    return Backup::Response::Ok;
}

Backup::ItemDescriptor* Backup::Process::GetRoot() {
    return mRoot;
}

void Backup::Process::PopulateItemDescriptor(Backup::ItemDescriptor* descriptor) {
    mOptions.Callback(Backup::State::Populating, "Populating item descriptor ID " + std::to_string(descriptor->Id), mProgress);
    auto asset_delivery = mCore->GetRegistry()->GetKeyValue<std::string>("internet.roblox.asset_delivery")
        .value_or("https://assetdelivery.roblox.com/v1/asset/?id={}");
    auto asset_details = mCore->GetRegistry()->GetKeyValue<std::string>("internet.roblox.asset_details")
        .value_or("https://economy.roblox.com/v2/assets/{}/details");
    auto badge_details = mCore->GetRegistry()->GetKeyValue<std::string>("internet.roblox.badge_details")
        .value_or("https://badges.roblox.com/v1/badges/{}");
    auto universe_details = mCore->GetRegistry()->GetKeyValue<std::string>("internet.roblox.universe_details")
        .value_or("https://games.roblox.com/v1/games?universeIds={}");
    auto universe_places = mCore->GetRegistry()->GetKeyValue<std::string>("internet.roblox.universe_places")
        .value_or("https://develop.roblox.com/v1/universes/{}/places");
    auto universe_badges = mCore->GetRegistry()->GetKeyValue<std::string>("internet.roblox.universe_badges")
        .value_or("https://badges.roblox.com/v1/universes/{}/badges");

    auto idStr = std::to_string(descriptor->Id);

    NetClient client;
    if (descriptor->Type == ItemType::Universe) {
        HttpRequest detailsReq = CreateRbxReq(mCore);
        detailsReq.Url = std::vformat(universe_details, std::make_format_args(idStr));
        HttpResponse detailsRes = client.Fetch(detailsReq);
        if (detailsRes.Code != CURLE_OK) {
            mOptions.Callback(Backup::State::DownloadingFailed, "Failed to retrieve universe details for ID " + idStr, mProgress);
            return;
        }
        try {
            mOptions.Callback(Backup::State::ParsingJson, "Attempting to parse details JSON for universe " + idStr, mProgress);
            nlohmann::json json = nlohmann::json::parse(detailsRes.Body);
            if (!json.contains("data")) {
                mOptions.Callback(Backup::State::ParsingJsonFailed, "Failed to parse details JSON for universe " + idStr + " because array \"data\" is not included.", mProgress);
                return;
            }
            for (auto& [index, array] : json["data"].items()) {
                if (array["name"].is_string())
                    descriptor->Name = array["name"].get<std::string>();
                if (array["name"].is_string())
                    descriptor->Description = array["description"].get<std::string>();
            }
        } catch (nlohmann::json::exception &ex) {
            mOptions.Callback(Backup::State::ParsingJsonFailed, "Failed to parse details JSON for universe " + idStr, mProgress);
        }

        HttpRequest placesReq = CreateRbxReq(mCore);
        placesReq.Url = std::vformat(universe_places, std::make_format_args(idStr));
        HttpResponse placesRes = client.Fetch(placesReq);
        if (placesRes.Code != CURLE_OK) {
            mOptions.Callback(Backup::State::DownloadingFailed, "Failed to download places JSON for universe " + idStr, mProgress);
            return;
        }
        try {
            mOptions.Callback(Backup::State::ParsingJson, "Attempting to parse places JSON for universe " + idStr, mProgress);
            nlohmann::json json = nlohmann::json::parse(placesRes.Body);
            if (!json.contains("data")) {
                mOptions.Callback(Backup::State::ParsingJsonFailed, "Failed to parse places JSON for universe " + idStr + " because array \"data\" is not included.", mProgress);
                return;
            }
            for (auto& [index, array] : json["data"].items()) {
                if (array["id"].is_number()) {
                    int64_t placeId = array["id"].get<int64_t>();
                    auto *childDesc = new ItemDescriptor();
                    childDesc->Type = ItemType::Asset;
                    childDesc->Id = placeId;
                    childDesc->Version = 0;
                    descriptor->AddChild(childDesc);
                    PopulateItemDescriptor(childDesc);
                }
            }
        } catch (nlohmann::json::exception &ex) {
            mOptions.Callback(Backup::State::ParsingJsonFailed, "Failed to parse places JSON for universe " + idStr, mProgress);
        }

        HttpRequest badgesReq = CreateRbxReq(mCore);
        badgesReq.Url = std::vformat(universe_badges, std::make_format_args(idStr));
        HttpResponse badgesRes = client.Fetch(badgesReq);
        if (badgesRes.Code != CURLE_OK) {
            mOptions.Callback(Backup::State::DownloadingFailed, "Failed to retrieve badges for universe " + idStr, mProgress);
            return;
        }
        try {
            mOptions.Callback(Backup::State::ParsingJson, "Attempting to parse badge JSON body", mProgress);
            nlohmann::json json = nlohmann::json::parse(badgesRes.Body);
            if (!json.contains("data")) {
                mOptions.Callback(Backup::State::ParsingJsonFailed, "Failed to parse badges JSON for universe " + idStr + " because array \"data\" is not included.", mProgress);
                return;
            }
            for (auto& [index, array] : json["data"].items()) {
                if (array["id"].is_number()) {
                    int64_t badgeId = array["id"].get<int64_t>();
                    auto *childDesc = new ItemDescriptor();
                    childDesc->Type = ItemType::Badge;
                    childDesc->Id = badgeId;
                    childDesc->Version = 0;
                    descriptor->AddChild(childDesc);
                    PopulateItemDescriptor(childDesc);
                }
            }
        } catch (nlohmann::json::exception &ex) {
            mOptions.Callback(Backup::State::ParsingJsonFailed, "Failed to parse badges JSON for universe " + idStr, mProgress);
        }
    } else if (descriptor->Type == ItemType::Asset) {
        HttpRequest assetReq = CreateRbxReq(mCore);
        assetReq.Url = std::vformat(asset_details, std::make_format_args(idStr));
        HttpResponse assetRes = client.Fetch(assetReq);
        if (assetRes.Code != CURLE_OK) {
            mOptions.Callback(Backup::State::DownloadingFailed, "Failed to retrieve asset details for ID " + idStr, mProgress);
            return;
        }
        try {
            mOptions.Callback(Backup::State::ParsingJson, "Attempting to parse asset JSON body", mProgress);
            nlohmann::json json = nlohmann::json::parse(assetRes.Body);
            if (json["Name"].is_string()) {
                std::string name = json["Name"].get<std::string>();
                descriptor->Name = name;
            }

            if (json["Description"].is_string()) {
                std::string desc = json["Description"].get<std::string>();
                descriptor->Description = desc;
            }
        } catch (nlohmann::json::exception &ex) {
            mOptions.Callback(Backup::State::ParsingJsonFailed, "Failed to parse asset JSON " + idStr, mProgress);
        }
    } else if (descriptor->Type == ItemType::Badge) {
        HttpRequest badgeReq = CreateRbxReq(mCore);
        badgeReq.Url = std::vformat(badge_details, std::make_format_args(idStr));
        HttpResponse badgeRes = client.Fetch(badgeReq);
        if (badgeRes.Code != CURLE_OK) {
            mOptions.Callback(Backup::State::DownloadingFailed, "Failed to retrieve badge details for ID " + idStr, mProgress);
            return;
        }
        try {
            mOptions.Callback(Backup::State::ParsingJson, "Attempting to parse badge JSON body", mProgress);
            nlohmann::json json = nlohmann::json::parse(badgeRes.Body);
            if (json["iconImageId"].is_number()) {
                int64_t iconId = json["iconImageId"].get<int64_t>();
                auto *childDesc = new ItemDescriptor();
                childDesc->Type = ItemType::Asset;
                childDesc->Id = iconId;
                childDesc->Version = 0;
                descriptor->AddChild(childDesc);
                PopulateItemDescriptor(childDesc);
            }

            if (json["name"].is_string()) {
                std::string name = json["name"].get<std::string>();
                descriptor->Name = name;
            }

            if (json["description"].is_string()) {
                std::string desc = json["description"].get<std::string>();
                descriptor->Description = desc;
            }
        } catch (nlohmann::json::exception &ex) {
            mOptions.Callback(Backup::State::ParsingJsonFailed, "Failed to parse badge JSON " + idStr, mProgress);
        }
    }
    /*switch (descriptor->Type) {
    case ItemType::Universe:
        req.Url = std::vformat(game_details_url, std::make_format_args(idStr));
        client.Fetch(req);
    default:
        break;
    }*/
}

void Backup::Process::DownloadItemDescriptorRecursively(Backup::ItemDescriptor* descriptor) {
    
}