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
#include <cpr/cpr.h>

#include <NoobWarrior/Backup.h>
#include <NoobWarrior/Registry.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Roblox/Api/Asset.h>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <iostream>
#include <filesystem>
#include <sstream>
#include <utility>

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

// Pull the filename out of a Content-Disposition header value, e.g.
// `attachment; filename="abc123"` -> `abc123`. Returns empty if not present.
static std::string FileNameFromContentDisposition(const std::string &header) {
    const std::string key = "filename=";
    size_t pos = header.find(key);
    if (pos == std::string::npos)
        return {};
    pos += key.size();
    std::string name = header.substr(pos);
    // Strip surrounding quotes and anything after a trailing separator.
    if (!name.empty() && name.front() == '"') {
        name.erase(0, 1);
        size_t end = name.find('"');
        if (end != std::string::npos)
            name.erase(end);
    } else {
        size_t end = name.find_first_of(";\r\n");
        if (end != std::string::npos)
            name.erase(end);
    }
    return name;
}

int Core::DownloadAssets(DownloadAssetArgs args) {
    if (args.OutStream == nullptr) args.OutStream = &std::cout;
    if (!std::filesystem::is_directory(args.OutDir)) {
        OutEx(args.OutStream, "AssetRequest", "Failed to download assets: Directory \"{}\" doesn't exist", args.OutDir);
        return -1;
    }
    for (int i = 0; i < args.Id.size(); i++) {
        int64_t id = args.Id.at(i);
        OutEx(args.OutStream, "AssetRequest", "Downloading ID {}", id);

        std::optional<std::string> download_url = GetRegistry()->GetKeyValue<std::string>("internet.roblox.asset_download");
        if (!download_url.has_value())
            return -2;

        std::string fmtApiCall = std::vformat(download_url.value(), std::make_format_args(id));

        cpr::Session session;
        session.SetUrl(cpr::Url{fmtApiCall});
        session.SetUserAgent(cpr::UserAgent{"Roblox/WinINet"}); // use the same user agent that the Roblox client uses.
        cpr::Response res = session.Get();

        if (res.error.code != cpr::ErrorCode::OK) {
            OutEx(args.OutStream, "AssetRequest", "Failed to download ID {}: Curl error {}, {}", id, (int)res.error.code, res.error.message);
            continue;
        }
        if (res.status_code != 200) {
            OutEx(args.OutStream, "AssetRequest", "Failed to download ID {}: {} {}", id, res.status_code, sHttpStatusMessages.count(res.status_code) ? sHttpStatusMessages.at(res.status_code) : "Unknown");
            continue;
        }

        // By default the file is named after the asset ID. For the Raw style we use the name the
        // server reports in its Content-Disposition header (an MD5 hash, for Roblox), if present.
        std::string fileName = std::to_string(id);
        if (args.FileNameStyle == AssetFileNameStyle::Raw) {
            auto it = res.header.find("Content-Disposition");
            if (it != res.header.end()) {
                std::string fromHeader = FileNameFromContentDisposition(it->second);
                if (!fromHeader.empty())
                    fileName = fromHeader;
            }
        }

        std::string fileDir = args.OutDir + "/" + fileName;
        FILE* filePointer = fopen(fileDir.c_str(), "wb");
        if (filePointer == NULL) {
            OutEx(args.OutStream, "AssetRequest", "Failed to download ID {}: Failed to create file", id);
            continue;
        }
        fwrite(res.text.data(), 1, res.text.size(), filePointer);
        fclose(filePointer);
    }
    OutEx(args.OutStream, "AssetRequest", !args.Id.empty() ? "Finished iterating through all IDs." : "Stopping, nothing to download.");
    return 1;
}

int Core::GetAssetDetails(int64_t id, Roblox::AssetDetails *details) {
    int ret = 0;
    std::string details_url = GetRegistry()->GetKeyValue<std::string>("internet.roblox.asset_details")
        .value_or("https://economy.roblox.com/v2/assets/{}/details");

    cpr::Session session;
    session.SetUrl(cpr::Url{std::vformat(details_url, std::make_format_args(id))});
    session.SetUserAgent(cpr::UserAgent{"Roblox/WinINet"});

    // The economy details endpoint requires authentication; without the
    // .ROBLOSECURITY cookie it returns an error payload instead of the details.
    if (auto *acc = GetRbxKeychain()->GetActiveAccount())
        session.SetHeader(cpr::Header{{"Cookie", ".ROBLOSECURITY=" + acc->Token + ";"}});

    cpr::Response res = session.Get();
    if (res.error.code == cpr::ErrorCode::OK) {
        json data = json::parse(res.text, nullptr, false);

        if (data.is_discarded() || (data.contains("errors") && !data["errors"].is_null())) {
            return -1;
        }

        auto getInt = [&](const json &j, const char *key) -> int64_t {
            return j.contains(key) && j[key].is_number() ? j[key].get<int64_t>() : 0;
        };
        auto getStr = [&](const json &j, const char *key) -> std::string {
            return j.contains(key) && j[key].is_string() ? j[key].get<std::string>() : std::string();
        };
        auto getBool = [&](const json &j, const char *key) -> bool {
            return j.contains(key) && j[key].is_boolean() ? j[key].get<bool>() : false;
        };

        const json &creator = data.contains("Creator") && data["Creator"].is_object() ? data["Creator"] : json::object();

        details->TargetId = getInt(data, "TargetId");
        details->ProductType = data.contains("ProductType") && data["ProductType"].is_string()
            ? (data["ProductType"] != "Collectible Item" ? Roblox::ProductType::UserProduct : Roblox::ProductType::CollectibleItem)
            : Roblox::ProductType::None;
        details->AssetId = getInt(data, "AssetId");
        details->ProductId = getInt(data, "ProductId");
        details->Name = getStr(data, "Name");
        details->Description = getStr(data, "Description");
        details->AssetType = static_cast<Roblox::AssetType>(getInt(data, "AssetTypeId"));
        details->CreatorId = getInt(creator, "Id");
        details->CreatorName = getStr(creator, "Name");
        details->CreatorType = getStr(creator, "CreatorType") != "Group" ? Roblox::CreatorType::User : Roblox::CreatorType::Group;
        details->CreatorTargetId = getInt(creator, "CreatorTargetId");
        details->CreatorHasVerifiedBadge = getBool(creator, "HasVerifiedBadge");
        details->IconImageAssetId = getInt(data, "IconImageAssetId");
        details->Created = getStr(data, "Created").empty() ? 0 : ConvertISO8601ToTimestamp(getStr(data, "Created"));
        details->Updated = getStr(data, "Updated").empty() ? 0 : ConvertISO8601ToTimestamp(getStr(data, "Updated"));
        details->PriceInRobux = data.contains("PriceInRobux") && data["PriceInRobux"].is_number() ? data["PriceInRobux"].get<int>() : -1;
        details->PriceInTickets = data.contains("PriceInTickets") && data["PriceInTickets"].is_number() ? data["PriceInTickets"].get<int>() : -1;
        details->Sales = getInt(data, "Sales");
        details->IsNew = getBool(data, "IsNew");
        details->IsForSale = getBool(data, "IsForSale");
        details->IsPublicDomain = getBool(data, "IsPublicDomain");
        details->IsLimited = getBool(data, "IsLimited");
        details->IsLimitedUnique = getBool(data, "IsLimitedUnique");
        details->MinimumMembershipLevel = static_cast<Roblox::MembershipType>(getInt(data, "MinimumMembershipLevel"));
        ret = 1;
    }
    return ret;
}

static std::string GetRbxCookie(Core* core) {
    if (auto *acc = core->GetRbxKeychain()->GetActiveAccount())
        return ".ROBLOSECURITY=" + acc->Token + ";";
    return {};
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
    std::map<std::pair<ItemType, int64_t>, bool> discoveredItems;
    PopulateItemDescriptor(mRoot, discoveredItems); // Phase 1: Populate the Item Descriptor with more Item Descriptors!!!
    DownloadItemDescriptorRecursively(mRoot); // Phase 2: Download them all!!!!!
    return Backup::Response::Ok;
}

Backup::ItemDescriptor* Backup::Process::GetRoot() {
    return mRoot;
}

void Backup::Process::PopulateItemDescriptor(Backup::ItemDescriptor* descriptor, std::map<std::pair<ItemType, int64_t>, bool> &discoveredItems) {
    discoveredItems[{ descriptor->Type, descriptor->Id }] = true;
    
    const std::string rbxCookie = GetRbxCookie(mCore);
    auto FetchJson = [&](
        const std::string& url,
        const std::string& description,
        nlohmann::json& out
    ) -> bool {
        cpr::Session session;
        session.SetUrl(cpr::Url{url});
        session.SetUserAgent(cpr::UserAgent{"Roblox/WinINet"});
        if (!rbxCookie.empty())
            session.SetHeader(cpr::Header{{"Cookie", rbxCookie}});
        curl_easy_setopt(session.GetCurlHolder()->handle, CURLOPT_SSL_OPTIONS, (long)CURLSSLOPT_NATIVE_CA);
        cpr::Response res = session.Get();

        if (res.error.code != cpr::ErrorCode::OK) {
            mOptions.Callback(Backup::State::DownloadingFailed,
                "Failed to download " + description, mProgress);
            return false;
        }
        try {
            out = nlohmann::json::parse(res.text);
            return true;
        } catch (const nlohmann::json::exception&) {
            mOptions.Callback(Backup::State::ParsingJsonFailed,
                "Failed to parse JSON for " + description, mProgress);
            return false;
        }
    };

    auto MakeChild = [&](ItemType type, int64_t id) -> ItemDescriptor* {
        if (discoveredItems.contains({ type, id })) {
            return nullptr;
        }
        auto* child = new ItemDescriptor();
        child->Type = type;
        child->Id = id;
        child->Version = 0;
        descriptor->AddChild(child);
        PopulateItemDescriptor(child, discoveredItems);
        return child;
    };

    mOptions.Callback(Backup::State::Populating,
        "Populating item descriptor ID " + std::to_string(descriptor->Id), mProgress);

    const std::string idStr = std::to_string(descriptor->Id);
    nlohmann::json json;

    if (descriptor->Type == ItemType::Universe) {
        auto universe_details = mCore->GetRegistry()->GetKeyValue<std::string>("internet.roblox.universe_details")
            .value_or("https://games.roblox.com/v1/games?universeIds={}");
        auto universe_places = mCore->GetRegistry()->GetKeyValue<std::string>("internet.roblox.universe_places")
            .value_or("https://develop.roblox.com/v1/universes/{}/places");
        auto universe_badges = mCore->GetRegistry()->GetKeyValue<std::string>("internet.roblox.universe_badges")
            .value_or("https://badges.roblox.com/v1/universes/{}/badges");

        if (FetchJson(std::vformat(universe_details, std::make_format_args(idStr)),
                    "universe details for " + idStr, json)
            && json.contains("data"))
        {
            for (auto& entry : json["data"]) {
                if (entry["name"].is_string())
                    descriptor->Name = entry["name"];
                if (entry["description"].is_string())
                    descriptor->Description = entry["description"];
                if (entry["creator"].is_array()) {
                    if (entry["creator"]["type"] == "User") {
                        MakeChild(ItemType::User, entry["creator"]["id"]);
                    } else if (entry["creator"]["type"] == "Group") {
                        MakeChild(ItemType::Group, entry["creator"]["id"]);
                    }
                }
            }
        }

        if (FetchJson(std::vformat(universe_places, std::make_format_args(idStr)),
                    "universe places for " + idStr, json)
            && json.contains("data"))
        {
            for (auto& entry : json["data"])
                if (entry["id"].is_number())
                    MakeChild(ItemType::Asset, entry["id"]);
        }

        if (FetchJson(std::vformat(universe_badges, std::make_format_args(idStr)),
                    "universe badges for " + idStr, json)
            && json.contains("data"))
        {
            for (auto& entry : json["data"])
                if (entry["id"].is_number())
                    MakeChild(ItemType::Badge, entry["id"]);
        }
    } else if (descriptor->Type == ItemType::Asset) {
        auto asset_details = mCore->GetRegistry()->GetKeyValue<std::string>("internet.roblox.asset_details")
            .value_or("https://economy.roblox.com/v2/assets/{}/details");

        if (FetchJson(std::vformat(asset_details, std::make_format_args(idStr)),
                    "asset " + idStr, json))
        {
            if (json["Name"].is_string())        descriptor->Name        = json["Name"];
            if (json["Description"].is_string()) descriptor->Description = json["Description"];
        }
    } else if (descriptor->Type == ItemType::Badge) {
        auto badge_details = mCore->GetRegistry()->GetKeyValue<std::string>("internet.roblox.badge_details")
            .value_or("https://badges.roblox.com/v1/badges/{}");

        if (FetchJson(std::vformat(badge_details, std::make_format_args(idStr)),
                    "badge " + idStr, json))
        {
            if (json["name"].is_string())
                descriptor->Name = json["name"];
            if (json["description"].is_string())
                descriptor->Description = json["description"];
            if (json["iconImageId"].is_number())
                MakeChild(ItemType::Asset, json["iconImageId"]);
        }
    } else if (descriptor->Type == ItemType::User) {
        auto user_details = mCore->GetRegistry()->GetKeyValue<std::string>("internet.roblox.user_details")
            .value_or("https://users.roblox.com/v1/users/{}");

        if (FetchJson(std::vformat(user_details, std::make_format_args(idStr)),
                    "user " + idStr, json))
        {
            if (json["name"].is_string())
                descriptor->Name = json["name"];
            if (json["description"].is_string())
                descriptor->Description = json["description"];
        }
    } else if (descriptor->Type == ItemType::Group) {
        auto group_details = mCore->GetRegistry()->GetKeyValue<std::string>("internet.roblox.group_details")
            .value_or("https://groups.roblox.com/v1/groups/{}");

        if (FetchJson(std::vformat(group_details, std::make_format_args(idStr)),
                    "group " + idStr, json))
        {
            if (json["name"].is_string())
                descriptor->Name = json["name"];
            if (json["description"].is_string())
                descriptor->Description = json["description"];
        }
    }
}

void Backup::Process::DownloadItemDescriptorRecursively(Backup::ItemDescriptor* descriptor) {
    
}