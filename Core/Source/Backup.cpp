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

#include "algorithm/gzip.h"

#include <iostream>
#include <filesystem>
#include <sstream>
#include <utility>
#include <thread>
#include <chrono>
#include <vector>
#include <cstdio>
#include <ctime>

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
    if (iso8601_string.empty())
        return -1;

    int year, mon, day, hour = 0, min = 0;
    double sec = 0.0;
    if (std::sscanf(iso8601_string.c_str(), "%d-%d-%dT%d:%d:%lf", &year, &mon, &day, &hour, &min, &sec) < 3)
        return -1;

    std::tm t{};
    t.tm_year = year - 1900;
    t.tm_mon  = mon - 1;
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min  = min;
    t.tm_sec  = static_cast<int>(sec);

#if defined(_WIN32)
    std::time_t epoch = _mkgmtime(&t);
#else
    std::time_t epoch = timegm(&t);
#endif
    if (epoch == -1)
        return -2;

    size_t tpos = iso8601_string.find('T');
    if (tpos != std::string::npos) {
        size_t off = iso8601_string.find_first_of("+-", tpos);
        if (off != std::string::npos) {
            int oh = 0, om = 0;
            std::sscanf(iso8601_string.c_str() + off + 1, "%2d:%2d", &oh, &om);
            int offsetSec = (oh * 3600 + om * 60) * (iso8601_string[off] == '-' ? -1 : 1);
            epoch -= offsetSec; // shift the local-offset time back to UTC
        }
    }

    return static_cast<long>(epoch);
}

static std::string FileNameFromContentDisposition(const std::string &header) {
    const std::string key = "filename=";
    size_t pos = header.find(key);
    if (pos == std::string::npos)
        return {};
    pos += key.size();
    std::string name = header.substr(pos);
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

        std::string download_url = GetRegistry()->GetKeyValue<std::string>("internet.roblox.asset_delivery")
            .value_or("https://assetdelivery.roblox.com/v1/asset/?id={}");

        std::string fmtApiCall = std::vformat(download_url, std::make_format_args(id));

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

        if (res.text.size() >= 2 && static_cast<unsigned char>(res.text[0]) == 0x1f
                                 && static_cast<unsigned char>(res.text[1]) == 0x8b) {
            std::vector<unsigned char> inflated =
                GzipInflate(reinterpret_cast<const unsigned char*>(res.text.data()), res.text.size());
            if (!inflated.empty())
                res.text.assign(inflated.begin(), inflated.end());
        }

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
    
    mAuthCookie = GetRbxCookie(core);

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


static cpr::Response BackupHttpGet(const std::string& url, const std::string& cookie) {
    cpr::Session session;
    session.SetUrl(cpr::Url{url});
    session.SetUserAgent(cpr::UserAgent{"Roblox/WinINet"});
    session.SetTimeout(cpr::Timeout{30000});
    session.SetConnectTimeout(cpr::ConnectTimeout{10000});
    if (!cookie.empty())
        session.SetHeader(cpr::Header{{"Cookie", cookie}});
    curl_easy_setopt(session.GetCurlHolder()->handle, CURLOPT_SSL_OPTIONS, (long)CURLSSLOPT_NATIVE_CA);

    cpr::Response res;
    for (int attempt = 0; attempt < 3; attempt++) {
        res = session.Get();
        bool transient = res.error.code != cpr::ErrorCode::OK || res.status_code == 429 || res.status_code == 500 || res.status_code == 502 || res.status_code == 503 || res.status_code == 504;
        if (!transient || attempt == 2)
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(300 * (attempt + 1)));
    }

    if (res.text.size() >= 2 && static_cast<unsigned char>(res.text[0]) == 0x1f && static_cast<unsigned char>(res.text[1]) == 0x8b) {
        std::vector<unsigned char> inflated =
            GzipInflate(reinterpret_cast<const unsigned char*>(res.text.data()), res.text.size());
        if (!inflated.empty())
            res.text.assign(inflated.begin(), inflated.end());
    }
    return res;
}

Backup::Response Backup::Process::Start() {
    if (mOptions.DestinationType == DestinationType::Database) {
        if (mOptions.Destination == nullptr) {
            Report(State::Failed, "No destination database was provided.");
            return Response::DestinationInvalid;
        }
    } else { // FileSystem
        const char* dir = static_cast<const char*>(mOptions.Destination);
        if (dir == nullptr || !std::filesystem::is_directory(dir)) {
            Report(State::Failed, "The output directory does not exist.");
            return Response::DestinationInvalid;
        }
    }

    mProgress = 0;
    Report(State::Started, "Started backup process");

    // Phase 1: walk the Roblox endpoints to discover everything that needs backing up.
    std::map<std::pair<ItemType, int64_t>, bool> discoveredItems;
    PopulateItemDescriptor(mRoot, discoveredItems);
    if (mCancelled) {
        Report(State::Failed, "Backup cancelled.");
        return Response::Cancelled;
    }

    // Phase 2: download and persist each discovered item.
    mTotalNodes = CountDescriptors(mRoot);
    mDoneNodes = 0;
    DownloadItemDescriptorRecursively(mRoot);
    if (mCancelled) {
        Report(State::Failed, "Backup cancelled.");
        return Response::Cancelled;
    }

    // Phase 3: optionally stamp the database's own metadata from the captured target.
    if (mOptions.SetDestinationMetaFromTarget && !mCancelled)
        SetDestinationMeta();

    mProgress = 1.0;
    Report(State::Success, "Backup complete.");
    return Response::Ok;
}

void Backup::Process::Cancel() {
    mCancelled.store(true);
}

void Backup::Process::Report(State state, const std::string& message) {
    if (mOptions.Callback)
        mOptions.Callback(state, message, mProgress);
}

void Backup::Process::RunDb(const std::function<void()>& work) {
    if (mOptions.DbExecutor)
        mOptions.DbExecutor(work);
    else
        work();
}

int Backup::Process::CountDescriptors(Backup::ItemDescriptor* descriptor) const {
    if (descriptor == nullptr)
        return 0;
    int count = 1;
    for (ItemDescriptor* child : descriptor->GetChildren())
        count += CountDescriptors(child);
    return count;
}

int64_t Backup::Process::FindParentUniverseId(Backup::ItemDescriptor* descriptor) const {
    for (ItemDescriptor* parent = descriptor->GetParent(); parent != nullptr; parent = parent->GetParent())
        if (parent->Type == ItemType::Universe)
            return parent->Id;
    return 0;
}

Backup::ItemDescriptor* Backup::Process::GetRoot() {
    return mRoot;
}

void Backup::Process::PopulateItemDescriptor(Backup::ItemDescriptor* descriptor, std::map<std::pair<ItemType, int64_t>, bool> &discoveredItems) {
    if (descriptor == nullptr || mCancelled)
        return;

    discoveredItems[{ descriptor->Type, descriptor->Id }] = true;

    auto FetchJson = [&](const std::string& url, const std::string& description, nlohmann::json& out) -> bool {
        cpr::Response res = BackupHttpGet(url, mAuthCookie);
        if (res.error.code != cpr::ErrorCode::OK) {
            Report(State::DownloadingFailed, "Failed to download " + description);
            return false;
        }
        out = nlohmann::json::parse(res.text, nullptr, false);
        if (out.is_discarded()) {
            Report(State::ParsingJsonFailed, "Failed to parse JSON for " + description);
            return false;
        }
        return true;
    };

    // Safe JSON accessors, these never insert into or throw on a missing/mistyped key
    auto jStr = [](const nlohmann::json& j, const char* key) -> std::string {
        auto it = j.find(key);
        return (it != j.end() && it->is_string()) ? it->get<std::string>() : std::string();
    };
    auto jNum = [](const nlohmann::json& j, const char* key) -> int64_t {
        auto it = j.find(key);
        return (it != j.end() && it->is_number()) ? it->get<int64_t>() : 0;
    };
    auto jTs = [](const nlohmann::json& j, const char* key) -> int64_t {
        auto it = j.find(key);
        if (it == j.end() || !it->is_string()) return 0;
        long ts = ConvertISO8601ToTimestamp(it->get<std::string>());
        return ts < 0 ? 0 : static_cast<int64_t>(ts);
    };

    auto MakeChild = [&](ItemType type, int64_t id, Roblox::AssetType assetType = Roblox::AssetType::None) -> ItemDescriptor* {
        if (id <= 0 || discoveredItems.contains({ type, id }))
            return nullptr;
        auto* child = new ItemDescriptor();
        child->Type = type;
        child->Id = id;
        child->Version = 0;
        child->AssetType = assetType;
        descriptor->AddChild(child);
        PopulateItemDescriptor(child, discoveredItems);
        return child;
    };

    auto Reg = [&](const char* key, const char* fallback) -> std::string {
        return mCore->GetRegistry()->GetKeyValue<std::string>(key).value_or(fallback);
    };

    Report(State::Populating, "Populating item descriptor ID " + std::to_string(descriptor->Id));

    const std::string idStr = std::to_string(descriptor->Id);
    nlohmann::json json;

    switch (descriptor->Type) {
    case ItemType::Universe: {
        std::string detailsUrl = std::vformat(Reg("internet.roblox.universe_details", "https://games.roblox.com/v1/games?universeIds={}"), std::make_format_args(idStr));
        std::string placesUrl  = std::vformat(Reg("internet.roblox.universe_places",  "https://develop.roblox.com/v1/universes/{}/places"), std::make_format_args(idStr));
        std::string badgesUrl  = std::vformat(Reg("internet.roblox.universe_badges",  "https://badges.roblox.com/v1/universes/{}/badges"), std::make_format_args(idStr));

        if (FetchJson(detailsUrl, "universe details for " + idStr, json) && json.contains("data") && json["data"].is_array()) {
            for (auto& entry : json["data"]) {
                descriptor->Name = jStr(entry, "name");
                descriptor->Description = jStr(entry, "description");
                descriptor->Created = jTs(entry, "created");
                descriptor->Updated = jTs(entry, "updated");
                descriptor->StartPlaceId = jNum(entry, "rootPlaceId");

                auto creatorIt = entry.find("creator");
                if (creatorIt != entry.end() && creatorIt->is_object()) {
                    int64_t creatorId = jNum(*creatorIt, "id");
                    bool isGroup = jStr(*creatorIt, "type") == "Group";
                    descriptor->CreatorId = creatorId;
                    descriptor->CreatorType = isGroup ? Roblox::CreatorType::Group : Roblox::CreatorType::User;
                    MakeChild(isGroup ? ItemType::Group : ItemType::User, creatorId);
                }
                break; // only one universe was requested
            }
        }
        if (mCancelled) return;

        // Each place is an asset of type Place; develop's listing doesn't give the type, so force it.
        // This endpoint is public, so it also doubles as the fallback source for the universe's name,
        // description and start place when the authenticated games endpoint above returned nothing.
        if (FetchJson(placesUrl, "universe places for " + idStr, json) && json.contains("data") && json["data"].is_array()) {
            bool firstPlace = true;
            for (auto& entry : json["data"]) {
                int64_t placeId = jNum(entry, "id");
                if (firstPlace) {
                    if (descriptor->StartPlaceId == 0)   descriptor->StartPlaceId = placeId;
                    if (descriptor->Name.empty())        descriptor->Name = jStr(entry, "name");
                    if (descriptor->Description.empty()) descriptor->Description = jStr(entry, "description");
                    firstPlace = false;
                }
                MakeChild(ItemType::Asset, placeId, Roblox::AssetType::Place);
            }
        }
        if (mCancelled) return;

        if (FetchJson(badgesUrl, "universe badges for " + idStr, json) && json.contains("data") && json["data"].is_array()) {
            for (auto& entry : json["data"])
                MakeChild(ItemType::Badge, jNum(entry, "id"));
        }
        break;
    }
    case ItemType::Asset: {
        std::string detailsUrl = std::vformat(Reg("internet.roblox.asset_details", "https://economy.roblox.com/v2/assets/{}/details"), std::make_format_args(idStr));
        if (FetchJson(detailsUrl, "asset " + idStr, json) && json.is_object()) {
            descriptor->Name = jStr(json, "Name");
            descriptor->Description = jStr(json, "Description");
            descriptor->Created = jTs(json, "Created");
            descriptor->Updated = jTs(json, "Updated");

            // Keep a type we were given when discovered (e.g. Place from a universe); otherwise adopt
            // the economy endpoint's AssetTypeId.
            int64_t assetTypeId = jNum(json, "AssetTypeId");
            if (assetTypeId > 0 && descriptor->AssetType == Roblox::AssetType::None)
                descriptor->AssetType = static_cast<Roblox::AssetType>(assetTypeId);

            auto creatorIt = json.find("Creator");
            if (creatorIt != json.end() && creatorIt->is_object()) {
                int64_t creatorId = jNum(*creatorIt, "CreatorTargetId");
                if (creatorId <= 0) creatorId = jNum(*creatorIt, "Id");
                bool isGroup = jStr(*creatorIt, "CreatorType") == "Group";
                descriptor->CreatorId = creatorId;
                descriptor->CreatorType = isGroup ? Roblox::CreatorType::Group : Roblox::CreatorType::User;
                MakeChild(isGroup ? ItemType::Group : ItemType::User, creatorId);
            }
            
            descriptor->ImageId = jNum(json, "IconImageAssetId");
            MakeChild(ItemType::Asset, descriptor->ImageId);
        }
        break;
    }
    case ItemType::Badge: {
        std::string detailsUrl = std::vformat(Reg("internet.roblox.badge_details", "https://badges.roblox.com/v1/badges/{}"), std::make_format_args(idStr));
        if (FetchJson(detailsUrl, "badge " + idStr, json) && json.is_object()) {
            descriptor->Name = jStr(json, "name");
            descriptor->Description = jStr(json, "description");
            descriptor->Created = jTs(json, "created");
            descriptor->Updated = jTs(json, "updated");
            // The badge's icon is a child image asset; point the badge's ImageId at it so the SDK's
            // RetrieveImageData (which resolves a badge's preview by following ImageId) finds it.
            descriptor->ImageId = jNum(json, "iconImageId");
            MakeChild(ItemType::Asset, descriptor->ImageId);
        }
        break;
    }
    case ItemType::User: {
        std::string detailsUrl = std::vformat(Reg("internet.roblox.user_details", "https://users.roblox.com/v1/users/{}"), std::make_format_args(idStr));
        if (FetchJson(detailsUrl, "user " + idStr, json) && json.is_object()) {
            descriptor->Name = jStr(json, "name");
            descriptor->Description = jStr(json, "description");
            descriptor->Created = jTs(json, "created");
        }
        break;
    }
    case ItemType::Group: {
        std::string detailsUrl = std::vformat(Reg("internet.roblox.group_details", "https://groups.roblox.com/v1/groups/{}"), std::make_format_args(idStr));
        if (FetchJson(detailsUrl, "group " + idStr, json) && json.is_object()) {
            descriptor->Name = jStr(json, "name");
            descriptor->Description = jStr(json, "description");
            descriptor->Created = jTs(json, "created");
            auto ownerIt = json.find("owner");
            if (ownerIt != json.end() && ownerIt->is_object())
                MakeChild(ItemType::User, jNum(*ownerIt, "userId"));
        }
        break;
    }
    case ItemType::Bundle: {
        std::string detailsUrl = std::vformat(Reg("internet.roblox.bundle_details", "https://catalog.roblox.com/v1/bundles/{}/details"), std::make_format_args(idStr));
        if (FetchJson(detailsUrl, "bundle " + idStr, json) && json.is_object()) {
            descriptor->Name = jStr(json, "name");
            descriptor->Description = jStr(json, "description");
            auto itemsIt = json.find("items");
            if (itemsIt != json.end() && itemsIt->is_array()) {
                for (auto& item : *itemsIt)
                    if (jStr(item, "type") == "Asset")
                        MakeChild(ItemType::Asset, jNum(item, "id"));
            }
        }
        break;
    }
    default:
        Report(State::Failed, "Unsupported item type for backup (id " + idStr + ")");
        break;
    }
}

bool Backup::Process::DownloadAssetData(int64_t id, const std::string& cookie, std::vector<unsigned char>& out) {
    std::string deliveryUrl = mCore->GetRegistry()->GetKeyValue<std::string>("internet.roblox.asset_delivery")
        .value_or("https://assetdelivery.roblox.com/v1/asset/?id={}");
    std::string idStr = std::to_string(id);
    cpr::Response res = BackupHttpGet(std::vformat(deliveryUrl, std::make_format_args(idStr)), cookie);
    if (res.error.code != cpr::ErrorCode::OK || res.status_code != 200 || res.text.empty())
        return false;
    out.assign(res.text.begin(), res.text.end());
    return true;
}

// Resolves a thumbnails.roblox.com JSON endpoint of the shape {"data":[{"state","imageUrl",...}]} to
// the first Completed entry's image bytes. Shared by the asset-thumbnail and game-icon fetchers.
static bool FetchRenderedThumbnail(const std::string& jsonUrl, std::vector<unsigned char>& out) {
    cpr::Response res = BackupHttpGet(jsonUrl, "");
    if (res.error.code != cpr::ErrorCode::OK || res.status_code != 200)
        return false;

    nlohmann::json root = nlohmann::json::parse(res.text, nullptr, false);
    if (root.is_discarded() || !root.contains("data") || !root["data"].is_array() || root["data"].empty())
        return false;

    const nlohmann::json& first = root["data"][0];
    auto stateIt = first.find("state");
    auto urlIt = first.find("imageUrl");
    if (stateIt == first.end() || !stateIt->is_string() || stateIt->get<std::string>() != "Completed")
        return false;
    if (urlIt == first.end() || !urlIt->is_string())
        return false;

    cpr::Response img = BackupHttpGet(urlIt->get<std::string>(), "");
    if (img.error.code != cpr::ErrorCode::OK || img.status_code != 200 || img.text.empty())
        return false;

    out.assign(img.text.begin(), img.text.end());
    return true;
}

bool Backup::Process::DownloadAssetThumbnail(int64_t id, std::vector<unsigned char>& out) {
    std::string thumbTmpl = mCore->GetRegistry()->GetKeyValue<std::string>("internet.roblox.asset_thumbnail")
        .value_or("https://thumbnails.roblox.com/v1/assets?assetIds={}&size=420x420&format=Png&isCircular=false");
    std::string idStr = std::to_string(id);
    return FetchRenderedThumbnail(std::vformat(thumbTmpl, std::make_format_args(idStr)), out);
}

bool Backup::Process::DownloadGameIcon(int64_t universeId, std::vector<unsigned char>& out) {
    std::string tmpl = mCore->GetRegistry()->GetKeyValue<std::string>("internet.roblox.universe_icon")
        .value_or("https://thumbnails.roblox.com/v1/games/icons?universeIds={}&size=512x512&format=Png&isCircular=false");
    std::string idStr = std::to_string(universeId);
    return FetchRenderedThumbnail(std::vformat(tmpl, std::make_format_args(idStr)), out);
}

bool Backup::Process::DownloadUserThumbnail(int64_t userId, std::vector<unsigned char>& out) {
    std::string tmpl = mCore->GetRegistry()->GetKeyValue<std::string>("internet.roblox.user_thumbnail")
        .value_or("https://thumbnails.roblox.com/v1/users/avatar-headshot?userIds={}&size=420x420&format=Png&isCircular=false");
    std::string idStr = std::to_string(userId);
    return FetchRenderedThumbnail(std::vformat(tmpl, std::make_format_args(idStr)), out);
}

bool Backup::Process::DownloadUserAvatar(int64_t userId, std::vector<unsigned char>& out) {
    std::string tmpl = mCore->GetRegistry()->GetKeyValue<std::string>("internet.roblox.user_avatar")
        .value_or("https://thumbnails.roblox.com/v1/users/avatar?userIds={}&size=420x420&format=Png&isCircular=false");
    std::string idStr = std::to_string(userId);
    return FetchRenderedThumbnail(std::vformat(tmpl, std::make_format_args(idStr)), out);
}

int64_t Backup::Process::ResolveUniverseForPlace(Backup::ItemDescriptor* descriptor) {
    // When the place was discovered under a universe, we already know it.
    int64_t universeId = FindParentUniverseId(descriptor);
    if (universeId != 0)
        return universeId;

    // Otherwise (a standalone place backup) ask Roblox which universe the place belongs to.
    std::string tmpl = mCore->GetRegistry()->GetKeyValue<std::string>("internet.roblox.place_universe")
        .value_or("https://apis.roblox.com/universes/v1/places/{}/universe");
    std::string idStr = std::to_string(descriptor->Id);
    cpr::Response res = BackupHttpGet(std::vformat(tmpl, std::make_format_args(idStr)), mAuthCookie);
    if (res.error.code != cpr::ErrorCode::OK || res.status_code != 200)
        return 0;
    nlohmann::json j = nlohmann::json::parse(res.text, nullptr, false);
    if (j.is_discarded())
        return 0;
    auto it = j.find("universeId");
    return (it != j.end() && it->is_number()) ? it->get<int64_t>() : 0;
}

void Backup::Process::DownloadPlaceThumbnails(Backup::ItemDescriptor* descriptor, EmuDb* db) {
    int64_t universeId = ResolveUniverseForPlace(descriptor);
    if (universeId == 0)
        return;

    // Fetch the universe's carousel once; subsequent places in the same universe reuse it.
    auto cached = mUniverseCarouselCache.find(universeId);
    if (cached == mUniverseCarouselCache.end()) {
        std::vector<std::pair<int64_t, std::vector<unsigned char>>> carousel;

        std::string tmpl = mCore->GetRegistry()->GetKeyValue<std::string>("internet.roblox.universe_thumbnails")
            .value_or("https://thumbnails.roblox.com/v1/games/multiget/thumbnails?universeIds={}&size=768x432&format=Png&countPerUniverse=25&defaults=true");
        std::string idStr = std::to_string(universeId);
        cpr::Response res = BackupHttpGet(std::vformat(tmpl, std::make_format_args(idStr)), "");
        if (res.error.code == cpr::ErrorCode::OK && res.status_code == 200) {
            nlohmann::json root = nlohmann::json::parse(res.text, nullptr, false);
            if (!root.is_discarded() && root.contains("data") && root["data"].is_array() && !root["data"].empty()) {
                auto thumbsIt = root["data"][0].find("thumbnails");
                if (thumbsIt != root["data"][0].end() && thumbsIt->is_array()) {
                    for (auto& t : *thumbsIt) {
                        if (mCancelled) break;
                        auto stateIt = t.find("state");
                        auto urlIt   = t.find("imageUrl");
                        auto tidIt   = t.find("targetId");
                        if (stateIt == t.end() || !stateIt->is_string() || stateIt->get<std::string>() != "Completed")
                            continue;
                        if (urlIt == t.end() || !urlIt->is_string())
                            continue;
                        int64_t targetId = (tidIt != t.end() && tidIt->is_number()) ? tidIt->get<int64_t>() : 0;
                        if (targetId == 0)
                            continue; // need the thumbnail's image id as the table key

                        cpr::Response img = BackupHttpGet(urlIt->get<std::string>(), "");
                        if (img.error.code == cpr::ErrorCode::OK && img.status_code == 200 && !img.text.empty())
                            carousel.emplace_back(targetId, std::vector<unsigned char>(img.text.begin(), img.text.end()));
                    }
                }
            }
        }
        cached = mUniverseCarouselCache.emplace(universeId, std::move(carousel)).first;
    }

    for (const auto& [targetId, blob] : cached->second) {
        if (mCancelled)
            return;
        if (blob.empty() || targetId == 0)
            continue;
        RunDb([&]() {
            // A carousel thumbnail's targetId is a real image asset id, so store it as its own Image
            // asset (carrying the rendered bytes) and link the place to it via AssetPlaceThumbnail.
            // RetrieveImageData then previews it like any other asset.
            if (!db->DoesItemExist(ItemType::Asset, targetId)) {
                SqlRow row;
                row.push_back({"Id", targetId});
                row.push_back({"Name", std::string("Game Thumbnail")});
                row.push_back({"Type", static_cast<int>(Roblox::AssetType::Image)});
                if (db->AddItem(ItemType::Asset, row) == SqlDb::Response::Success)
                    db->AttachDataToAsset(targetId, 0, blob);
            }
            db->AddThumbnailToPlace(descriptor->Id, targetId);
            db->MarkDirty();
        });
    }
}

void Backup::Process::SetDestinationMeta() {
    if (mOptions.DestinationType != DestinationType::Database || mOptions.Destination == nullptr || mRoot == nullptr)
        return;
    EmuDb* db = static_cast<EmuDb*>(mOptions.Destination);
    
    std::vector<unsigned char> icon;
    switch (mRoot->Type) {
    case ItemType::Universe:
        DownloadGameIcon(mRoot->Id, icon);
        break;
    case ItemType::User:
        if (!DownloadUserAvatar(mRoot->Id, icon))
            DownloadUserThumbnail(mRoot->Id, icon);
        break;
    default: // Asset (place/model), Badge, Bundle, ...
        DownloadAssetThumbnail(mRoot->Id, icon);
        break;
    }

    const std::string title = mRoot->Name;
    const std::string description = mRoot->Description;
    const int64_t creatorId = mRoot->CreatorId;
    const ItemType creatorType = mRoot->CreatorType == Roblox::CreatorType::Group ? ItemType::Group : ItemType::User;

    RunDb([&]() {
        if (!title.empty())       db->SetTitle(title);
        if (!description.empty()) db->SetDescription(description);
        if (creatorId > 0) {
            if (auto name = db->GetItemName(creatorType, creatorId); name.has_value() && !name->empty())
                db->SetAuthor(*name);
        }
        if (!icon.empty())
            db->SetIcon(icon);
        db->MarkDirty();
    });
}

SqlRow Backup::Process::BuildItemRow(Backup::ItemDescriptor* d, bool includeId) {
    SqlRow row;
    if (includeId)
        row.push_back({"Id", static_cast<int64_t>(d->Id)});

    const std::string name = d->Name.empty() ? std::to_string(d->Id) : d->Name;
    const char* creatorCol = d->CreatorType == Roblox::CreatorType::Group ? "GroupId" : "UserId";

    switch (d->Type) {
    case ItemType::Asset:
        row.push_back({"Name", name});
        row.push_back({"Type", static_cast<int>(d->AssetType)});
        if (!d->Description.empty())
            row.push_back({"Description", d->Description});
        if (d->Created > 0)
            row.push_back({"Created", d->Created});
        if (d->Updated > 0)
            row.push_back({"Updated", d->Updated});
        if (d->ImageId > 0)
            row.push_back({"ImageId", d->ImageId});
        if (d->CreatorId > 0)
            row.push_back({creatorCol, d->CreatorId});
        break;
    case ItemType::Universe:
        row.push_back({"Name", name});
        if (!d->Description.empty())
            row.push_back({"Description", d->Description});
        if (d->Created > 0)
            row.push_back({"Created", d->Created});
        if (d->Updated > 0)
            row.push_back({"Updated", d->Updated});
        if (d->StartPlaceId > 0)
            row.push_back({"StartPlaceId", d->StartPlaceId});
        if (d->CreatorId > 0)
            row.push_back({creatorCol, d->CreatorId});
        break;
    case ItemType::Badge:
        row.push_back({"Name", name});
        if (!d->Description.empty())
            row.push_back({"Description", d->Description});
        if (d->Created > 0)
            row.push_back({"Created", d->Created});
        if (d->Updated > 0)
            row.push_back({"Updated", d->Updated});
        if (d->ImageId > 0)
            row.push_back({"ImageId", d->ImageId});
        row.push_back({"UniverseId", FindParentUniverseId(d)});
        break;
    case ItemType::User:
        row.push_back({"Name", name});
        if (!d->Description.empty()) row.push_back({"Bio", d->Description});
        break;
    case ItemType::Group:
        row.push_back({"Name", name});
        if (!d->Description.empty()) row.push_back({"Description", d->Description});
        if (d->Created > 0)          row.push_back({"Created", d->Created});
        break;
    case ItemType::Bundle:
        row.push_back({"Name", name});
        if (!d->Description.empty()) row.push_back({"Description", d->Description});
        row.push_back({"Type", 0}); // Bundle.Type (bundleType) is NOT NULL
        break;
    default:
        return {};
    }
    return row;
}

void Backup::Process::DownloadItemDescriptor(Backup::ItemDescriptor* descriptor) {
    const std::string idStr = std::to_string(descriptor->Id);
    const std::string label = (descriptor->Name.empty() ? idStr : descriptor->Name) + " (" + idStr + ")";
    Report(State::Downloading, "Backing up " + label);

    const std::string& cookie = mAuthCookie;

    if (mOptions.DestinationType == DestinationType::Database) {
        EmuDb* db = static_cast<EmuDb*>(mOptions.Destination);

        // 1. metadata row (insert or update if the item is already present)
        if (mOptions.DownloadMetadata) {
            SqlRow insertRow = BuildItemRow(descriptor, true);
            if (!insertRow.empty()) {
                bool ok = true;
                RunDb([&]() {
                    SqlDb::Response res = db->DoesItemExist(descriptor->Type, descriptor->Id)
                        ? db->UpdateItem(descriptor->Type, descriptor->Id, BuildItemRow(descriptor, false))
                        : db->AddItem(descriptor->Type, insertRow);
                    if (res == SqlDb::Response::Success) db->MarkDirty();
                    else ok = false;
                });
                if (!ok)
                    Report(State::AddingToDatabaseFailed, "Failed to write metadata for " + label);
            }
        }

        // 2. only assets carry downloadable binary content and thumbnails
        if (descriptor->Type == ItemType::Asset) {
            std::vector<unsigned char> data;
            if (DownloadAssetData(descriptor->Id, cookie, data) && !data.empty()) {
                bool ok = true;
                RunDb([&]() {
                    if (db->AttachDataToAsset(descriptor->Id, 0, data) == SqlDb::Response::Success) db->MarkDirty();
                    else ok = false;
                });
                if (!ok)
                    Report(State::AddingToDatabaseFailed, "Failed to store data for " + label);
            } else {
                Report(State::DownloadingFailed, "Could not download data for " + label);
            }
            
            if (descriptor->AssetType == Roblox::AssetType::Place) {
                int64_t universeId = FindParentUniverseId(descriptor);
                if (universeId != 0)
                    RunDb([&]() { db->AddPlaceToUniverse(universeId, descriptor->Id); });
            }

            if (mOptions.DownloadAutoGeneratedThumbnails && !mCancelled) {
                std::vector<unsigned char> thumb;
                if (DownloadAssetThumbnail(descriptor->Id, thumb) && !thumb.empty()) {
                    RunDb([&]() {
                        if (db->AttachThumbnailDataToAsset(descriptor->Id, thumb) == SqlDb::Response::Success)
                            db->MarkDirty();
                    });
                }
            }

            if (descriptor->AssetType == Roblox::AssetType::Place && mOptions.DownloadAutoGeneratedThumbnails && !mCancelled)
                DownloadPlaceThumbnails(descriptor, db);
        }

        // 3. users carry avatar renders: a full-body shot (the preview) and a headshot (extra data)
        if (descriptor->Type == ItemType::User && mOptions.DownloadAutoGeneratedThumbnails && !mCancelled) {
            std::vector<unsigned char> bodyShot;
            if (DownloadUserAvatar(descriptor->Id, bodyShot) && !bodyShot.empty()) {
                RunDb([&]() {
                    if (db->AttachBodyShotToUser(descriptor->Id, bodyShot) == SqlDb::Response::Success)
                        db->MarkDirty();
                });
            }

            std::vector<unsigned char> headshot;
            if (!mCancelled && DownloadUserThumbnail(descriptor->Id, headshot) && !headshot.empty()) {
                RunDb([&]() {
                    if (db->AttachHeadshotToUser(descriptor->Id, headshot) == SqlDb::Response::Success)
                        db->MarkDirty();
                });
            }
        }
    } else { // FileSystem
        const std::filesystem::path baseDir(static_cast<const char*>(mOptions.Destination));

        if (descriptor->Type == ItemType::Asset) {
            std::vector<unsigned char> data;
            if (DownloadAssetData(descriptor->Id, cookie, data) && !data.empty()) {
                std::filesystem::path outPath = baseDir / idStr;
                if (FILE* fp = fopen(outPath.string().c_str(), "wb")) {
                    fwrite(data.data(), 1, data.size(), fp);
                    fclose(fp);
                } else {
                    Report(State::Failed, "Could not write file for " + label);
                }
            } else {
                Report(State::DownloadingFailed, "Could not download data for " + label);
            }
        }

        if (mOptions.DownloadMetadata) {
            nlohmann::json meta;
            meta["id"] = descriptor->Id;
            meta["type"] = GetTableNameFromItemType(descriptor->Type);
            meta["name"] = descriptor->Name;
            meta["description"] = descriptor->Description;
            if (descriptor->Type == ItemType::Asset)
                meta["assetType"] = static_cast<int>(descriptor->AssetType);
            const std::string metaStr = meta.dump(2);
            std::filesystem::path metaPath = baseDir / (idStr + ".json");
            if (FILE* fp = fopen(metaPath.string().c_str(), "wb")) {
                fwrite(metaStr.data(), 1, metaStr.size(), fp);
                fclose(fp);
            }
        }
    }
}

void Backup::Process::DownloadItemDescriptorRecursively(Backup::ItemDescriptor* descriptor) {
    if (descriptor == nullptr || mCancelled)
        return;

    DownloadItemDescriptor(descriptor);

    mDoneNodes++;
    mProgress = mTotalNodes > 0 ? static_cast<double>(mDoneNodes) / mTotalNodes : 1.0;

    for (ItemDescriptor* child : descriptor->GetChildren()) {
        if (mCancelled)
            return;
        DownloadItemDescriptorRecursively(child);
    }
}