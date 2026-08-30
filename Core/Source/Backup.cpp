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
#include <NoobWarrior/Roblox/DataType/BrickColor.h>
#include <NoobWarrior/EmuDb/AssetEnricher.h> // AssetTypeFromApiString
#include <NoobWarrior/Roblox/FileFormat/RobloxFile.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/Content.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/ContentId.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/ProtectedString.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/SharedString.h>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include "algorithm/gzip.h"

#include <iostream>
#include <filesystem>
#include <memory>
#include <sstream>
#include <utility>
#include <thread>
#include <chrono>
#include <vector>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string_view>
#include <algorithm>

using namespace NoobWarrior;
using json = nlohmann::json;

static constexpr int kBackupDepthHardCap = 20;
static constexpr std::size_t kMaxBackupNodes = 10000;

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
    // The header is server-controlled: keep only the basename so "..\\evil" can't escape the
    // output directory.
    size_t slash = name.find_last_of("/\\");
    if (slash != std::string::npos)
        name.erase(0, slash + 1);
    if (name.find_first_not_of('.') == std::string::npos)
        return {};
    return name;
}

// 8/30/26: This is old as shit. Does this even work anymore?? Do we even need this??
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
        // 8/30/26: Why are we not using ofstream?
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
        details->Created = getStr(data, "Created").empty() ? 0 : ParseIso8601Utc(getStr(data, "Created"));
        details->Updated = getStr(data, "Updated").empty() ? 0 : ParseIso8601Utc(getStr(data, "Updated"));
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

    mMaxDepth = std::clamp(core->GetRegistry()->GetKeyValue<int>("backup.max_depth").value_or(6),
                           1, kBackupDepthHardCap);

    ItemDescriptor* root = new ItemDescriptor();
    mRoot = root;
    if (mOptions.TargetSource == ItemSource::LocalFile) {
        std::filesystem::path filePath(mOptions.TargetFilePath);
        mRoot->Type = ItemType::Asset;
        mRoot->Id = 0;
        mRoot->Name = filePath.filename().string();
        std::string ext = filePath.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
        if (ext == ".rbxl" || ext == ".rbxlx")
            mRoot->AssetType = Roblox::AssetType::Place;
        else if (ext == ".rbxm" || ext == ".rbxmx")
            mRoot->AssetType = Roblox::AssetType::Model;
    } else {
        // Give it something to start off
        mRoot->Type = options.TargetItemType;
        mRoot->Id = options.TargetId;
    }
}

Backup::Process::~Process() {
    if (mRoot != nullptr) {
        NOOBWARRIOR_FREE_PTR(mRoot)
    }
}

Backup::ItemDescriptor* Backup::Process::ReleaseRoot() {
    ItemDescriptor* root = mRoot;
    mRoot = nullptr;
    return root;
}


// `cancelled` aborts in-flight transfers via the curl progress callback, skips retries and
// cuts backoff sleeps short.
static cpr::Response BackupHttpGet(const std::string& url, const std::string& cookie,
                                   const std::atomic<bool>* cancelled = nullptr) {
    cpr::Session session;
    session.SetUrl(cpr::Url{url});
    session.SetUserAgent(cpr::UserAgent{"Roblox/WinINet"});
    session.SetTimeout(cpr::Timeout{30000});
    session.SetConnectTimeout(cpr::ConnectTimeout{10000});
    if (!cookie.empty())
        session.SetHeader(cpr::Header{{"Cookie", cookie}});
    if (cancelled != nullptr)
        session.SetProgressCallback(cpr::ProgressCallback{
            [cancelled](cpr::cpr_pf_arg_t, cpr::cpr_pf_arg_t, cpr::cpr_pf_arg_t, cpr::cpr_pf_arg_t,
                        intptr_t) { return !cancelled->load(); }});
    curl_easy_setopt(session.GetCurlHolder()->handle, CURLOPT_SSL_OPTIONS, (long)CURLSSLOPT_NATIVE_CA);

    cpr::Response res;
    const int kMaxAttempts = 5;
    for (int attempt = 0; attempt < kMaxAttempts; attempt++) {
        res = session.Get();
        if (cancelled != nullptr && cancelled->load())
            break;
        bool transient = res.error.code != cpr::ErrorCode::OK || res.status_code == 429 ||
                         res.status_code == 500 || res.status_code == 502 ||
                         res.status_code == 503 || res.status_code == 504;
        if (!transient || attempt == kMaxAttempts - 1)
            break;

        // Honor Retry-After when present, else back off exponentially.
        long waitMs = 400L * (1L << attempt); // 400, 800, 1600, 3200
        if (auto it = res.header.find("Retry-After"); it != res.header.end()) {
            long secs = std::strtol(it->second.c_str(), nullptr, 10);
            if (secs > 0)
                waitMs = std::min<long>(secs * 1000L, 10000L);
        }
        // Sleep in slices so a Cancel doesn't have to wait out the whole backoff.
        for (long slept = 0; slept < waitMs; slept += 50) {
            if (cancelled != nullptr && cancelled->load())
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(std::min<long>(50, waitMs - slept)));
        }
        if (cancelled != nullptr && cancelled->load())
            break;
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

    // Negative progress renders the indeterminate bar; discovery has no percentage yet.
    mProgress = -1;
    Report(State::Started, "Started backup process");
    mDiscoveredItems.clear();

    // Phase 1: discover everything that needs backing up.
    if (mOptions.TargetSource == ItemSource::LocalFile) {
        std::vector<unsigned char> data;
        if (FILE* fp = fopen(mOptions.TargetFilePath.c_str(), "rb")) {
            fseek(fp, 0, SEEK_END);
            long size = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            if (size > 0) {
                data.resize(static_cast<size_t>(size));
                if (fread(data.data(), 1, data.size(), fp) != data.size())
                    data.clear();
            }
            fclose(fp);
        }
        if (data.empty()) {
            Report(State::Failed, "Could not read the source file.");
            return Response::SourceInvalid;
        }
        if (!ParseFileForAssetReferences(mRoot, data)) {
            Report(State::Failed, "The source file is not a readable Roblox place or model.");
            return Response::SourceInvalid;
        }
    } else {
        PopulateItemDescriptor(mRoot);
    }
    if (mDiscoveredItems.size() >= kMaxBackupNodes)
        Report(State::Populating, "Reached the backup item limit (" + std::to_string(kMaxBackupNodes) +
                                  "); some related items were skipped.");
    if (mCancelled) {
        Report(State::Failed, "Backup cancelled.");
        return Response::Cancelled;
    }

    // Phase 2: download and persist each discovered item.
    mProgress = 0;
    mTotalNodes = CountDescriptors(mRoot);
    mDoneNodes = 0;
    DownloadItemDescriptorRecursively(mRoot);
    if (mCancelled) {
        Report(State::Failed, "Backup cancelled.");
        return Response::Cancelled;
    }

    // Phase 3: stamp the database's own metadata. Not at progress 1.0, completing the toast arms
    // auto-dismiss, and dismissal doubles as cancel.
    if (mOptions.SetDestinationMetaFromTarget && !mCancelled) {
        mProgress = -1;
        Report(State::Finalizing, "Writing destination metadata");
        SetDestinationMeta();
    }

    mProgress = 1.0;
    const int failed = mFailedDownloads.load() + mFailedWrites.load();
    if (failed > 0)
        Report(State::Success, "Backup finished with " + std::to_string(failed) + " errors (" +
                               std::to_string(mTotalNodes) + " items).");
    else
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

void Backup::Process::PopulateItemDescriptor(Backup::ItemDescriptor* descriptor, int depth) {
    if (descriptor == nullptr || mCancelled)
        return;

    mDiscoveredItems.insert({ descriptor->Type, descriptor->Id });

    auto FetchJson = [&](const std::string& url, const std::string& description, nlohmann::json& out) -> bool {
        cpr::Response res = BackupHttpGet(url, mAuthCookie, &mCancelled);
        if (res.error.code != cpr::ErrorCode::OK) {
            mFailedDownloads++;
            Report(State::DownloadingFailed, "Failed to download " + description);
            return false;
        }
        // A non-200 (notably 429) still returns a JSON error body that would parse as a "successful"
        // object with no fields; treat it as failure.
        if (res.status_code != 200) {
            if (res.status_code == 429) mRatelimitedHits++;
            mFailedDownloads++;
            Report(res.status_code == 429 ? State::Ratelimited : State::DownloadingFailed,
                   "HTTP " + std::to_string(res.status_code) + " while downloading " + description);
            return false;
        }
        out = nlohmann::json::parse(res.text, nullptr, false);
        if (out.is_discarded()) {
            mFailedDownloads++;
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
        int64_t ts = ParseIso8601Utc(it->get<std::string>());
        return ts < 0 ? 0 : ts;
    };

    auto MakeChild = [&](ItemType type, int64_t id, Roblox::AssetType assetType = Roblox::AssetType::None) -> ItemDescriptor* {
        if (id <= 0 || mDiscoveredItems.contains({ type, id }) || mDiscoveredItems.size() >= kMaxBackupNodes)
            return nullptr;
        auto* child = new ItemDescriptor();
        child->Type = type;
        child->Id = id;
        child->AssetType = assetType;
        // Mark discovered up-front so a depth-capped (unpopulated) child still de-duplicates.
        mDiscoveredItems.insert({ type, id });
        descriptor->AddChild(child);
        // Past the depth cap the child is still recorded and downloaded, but descent stops.
        if (depth + 1 < mMaxDepth)
            PopulateItemDescriptor(child, depth + 1);
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

        // develop's listing gives no type, so force Place; this public endpoint also doubles as the
        // fallback for the universe's name/description/start place.
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
        if (mCancelled) return;

        // Game passes; the listing's name/price stand in when the stingier per-pass endpoint fails.
        std::string passesUrl = std::vformat(Reg("internet.roblox.universe_passes", "https://games.roblox.com/v1/games/{}/game-passes?limit=100&sortOrder=Asc"), std::make_format_args(idStr));
        if (FetchJson(passesUrl, "universe game passes for " + idStr, json) && json.contains("data") && json["data"].is_array()) {
            for (auto& entry : json["data"]) {
                ItemDescriptor* pass = MakeChild(ItemType::Pass, jNum(entry, "id"));
                if (pass != nullptr) {
                    if (pass->Name.empty()) pass->Name = jStr(entry, "name");
                    if (pass->Price == 0)   pass->Price = jNum(entry, "price");
                }
                if (mCancelled) return;
            }
        }

        // Developer products: paginated, shapes vary (bare array vs wrappers, camelCase vs PascalCase).
        std::string dpTmpl = Reg("internet.roblox.universe_devproducts", "https://apis.roblox.com/developer-products/v1/universes/{}/developerproducts?pageNumber={}&pageSize=50");
        for (int page = 1; page <= 4 && !mCancelled; page++) {
            std::string pageStr = std::to_string(page);
            nlohmann::json dpJson;
            if (!FetchJson(std::vformat(dpTmpl, std::make_format_args(idStr, pageStr)), "developer products for " + idStr, dpJson))
                break;
            const nlohmann::json* list = nullptr;
            if (dpJson.is_array())
                list = &dpJson;
            else if (auto it = dpJson.find("developerProducts"); it != dpJson.end() && it->is_array())
                list = &*it;
            else if (auto it = dpJson.find("data"); it != dpJson.end() && it->is_array())
                list = &*it;
            if (list == nullptr || list->empty())
                break;
            for (auto& entry : *list) {
                ItemDescriptor* product = MakeChild(ItemType::DevProduct, jNum(entry, "id"));
                if (product != nullptr) {
                    if (product->Name.empty())        product->Name = !jStr(entry, "name").empty() ? jStr(entry, "name") : jStr(entry, "Name");
                    if (product->Description.empty()) product->Description = !jStr(entry, "description").empty() ? jStr(entry, "description") : jStr(entry, "Description");
                    if (product->Price == 0)          product->Price = jNum(entry, "price") != 0 ? jNum(entry, "price") : jNum(entry, "PriceInRobux");
                    if (product->ImageId == 0)        product->ImageId = jNum(entry, "iconImageAssetId") != 0 ? jNum(entry, "iconImageAssetId") : jNum(entry, "IconImageAssetId");
                    if (product->ImageId > 0)
                        MakeChild(ItemType::Asset, product->ImageId);
                }
                if (mCancelled) return;
            }
            if (list->size() < 50)
                break; // short page = last page
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

            // Keep a type we were given at discovery; otherwise adopt the economy endpoint's AssetTypeId.
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
            // Point the badge's ImageId at its child image asset so RetrieveImageData resolves it.
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
        if (mCancelled) return;

        // Only the backup root pulls the avatar sub-graph; transitive creators must not, or the
        // worn-asset -> creator -> worn-assets chain overflows the stack.
        if (descriptor == mRoot) {
        // Prefer the v2 endpoint: v1 users/{id}/avatar 429s aggressively with a minutes-long penalty;
        // both response shapes are parsed.
        std::string fetchUrl = std::vformat(Reg("internet.roblox.avatar_fetch", "https://avatar.roblox.com/v2/avatar/avatar-fetch?userId={}&placeId=1"), std::make_format_args(idStr));
        std::string v1Url    = std::vformat(Reg("internet.roblox.user_avatar_details", "https://avatar.roblox.com/v1/users/{}/avatar"), std::make_format_args(idStr));
        nlohmann::json avatar;
        int failedBeforeAvatar = mFailedDownloads.load();
        bool gotAvatar = FetchJson(fetchUrl, "avatar for user " + idStr, avatar) && avatar.is_object();
        if (!gotAvatar)
            gotAvatar = FetchJson(v1Url, "avatar (v1) for user " + idStr, avatar) && avatar.is_object();
        if (gotAvatar)
            mFailedDownloads.store(failedBeforeAvatar); // a v2 miss recovered by the v1 fallback is not a loss
        if (gotAvatar) {
            descriptor->HasAvatar = true;

            // Worn assets: v2 -> assetAndAssetTypeIds [{assetId, assetTypeId}];
            //              v1 -> assets [{id, assetType:{id}}].
            auto addWorn = [&](int64_t assetId, int64_t assetTypeId) {
                if (assetId <= 0)
                    return;
                descriptor->WornAssetIds.push_back(assetId);
                MakeChild(ItemType::Asset, assetId, static_cast<Roblox::AssetType>(assetTypeId));
            };
            if (auto it = avatar.find("assetAndAssetTypeIds"); it != avatar.end() && it->is_array()) {
                for (auto& e : *it) {
                    addWorn(jNum(e, "assetId"), jNum(e, "assetTypeId"));
                    if (mCancelled) return;
                }
            } else if (auto it = avatar.find("assets"); it != avatar.end() && it->is_array()) {
                for (auto& e : *it) {
                    int64_t typeId = 0;
                    if (auto t = e.find("assetType"); t != e.end() && t->is_object())
                        typeId = jNum(*t, "id");
                    addWorn(jNum(e, "id"), typeId);
                    if (mCancelled) return;
                }
            }

            auto scalesIt = avatar.find("scales");
            if (scalesIt != avatar.end() && scalesIt->is_object()) {
                auto jDbl = [](const nlohmann::json& j, const char* key) -> double {
                    auto it = j.find(key);
                    return (it != j.end() && it->is_number()) ? it->get<double>() : 0.0;
                };
                descriptor->AvatarWidth       = jDbl(*scalesIt, "width");
                descriptor->AvatarHeight      = jDbl(*scalesIt, "height");
                descriptor->AvatarHead        = jDbl(*scalesIt, "head");
                descriptor->AvatarProportions = jDbl(*scalesIt, "proportion");
            }
            std::string rig = jStr(avatar, "resolvedAvatarType");
            if (rig.empty()) rig = jStr(avatar, "playerAvatarType");
            descriptor->AvatarBodyType = rig == "R15" ? 1 : 0;

            // Body colors: v1 -> integer ids in bodyColors{headColorId,...};
            //              v2 -> hex in bodyColor3s{headColor3,...}. Store packed 0xRRGGBB.
            const nlohmann::json* colorsObj = nullptr;
            const nlohmann::json* color3sObj = nullptr;
            if (auto it = avatar.find("bodyColors"); it != avatar.end() && it->is_object())
                colorsObj = &*it;
            if (auto it = avatar.find("bodyColor3s"); it != avatar.end() && it->is_object())
                color3sObj = &*it;
            auto packHex = [](const std::string& hex, int& out) -> bool {
                std::string h = hex;
                if (!h.empty() && h.front() == '#') h.erase(0, 1);
                if (h.size() != 6) return false;
                try { out = static_cast<int>(std::stoul(h, nullptr, 16)) & 0xFFFFFF; return true; }
                catch (...) { return false; }
            };
            struct ColorSlot { const char* idKey; const char* hexKey; UserCharacterBodyPart part; };
            const ColorSlot parts[] = {
                {"headColorId",     "headColor3",     UserCharacterBodyPart::Head},
                {"torsoColorId",    "torsoColor3",    UserCharacterBodyPart::Torso},
                {"rightArmColorId", "rightArmColor3", UserCharacterBodyPart::RightArm},
                {"leftArmColorId",  "leftArmColor3",  UserCharacterBodyPart::LeftArm},
                {"rightLegColorId", "rightLegColor3", UserCharacterBodyPart::RightLeg},
                {"leftLegColorId",  "leftLegColor3",  UserCharacterBodyPart::LeftLeg},
            };
            for (const auto& slot : parts) {
                int packed = -1;
                if (colorsObj) {
                    int64_t num = jNum(*colorsObj, slot.idKey);
                    if (num > 0) packed = Roblox::BrickColor::PackedRgbForNumber(static_cast<int>(num));
                }
                if (packed < 0) {
                    std::string hex;
                    if (color3sObj) hex = jStr(*color3sObj, slot.hexKey);
                    if (hex.empty() && colorsObj) hex = jStr(*colorsObj, slot.hexKey);
                    int fromHex = 0;
                    if (!hex.empty() && packHex(hex, fromHex)) packed = fromHex;
                }
                if (packed >= 0)
                    descriptor->BodyColors.emplace_back(static_cast<int>(slot.part), packed);
            }
        }
        } // end: only the root user fetches the avatar sub-graph
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
    case ItemType::Pass: {
        // Same PascalCase shape as the economy asset-details endpoint.
        std::string detailsUrl = std::vformat(Reg("internet.roblox.pass_details", "https://apis.roblox.com/game-passes/v1/game-passes/{}/product-info"), std::make_format_args(idStr));
        if (FetchJson(detailsUrl, "game pass " + idStr, json) && json.is_object()) {
            descriptor->Name = jStr(json, "Name");
            descriptor->Description = jStr(json, "Description");
            descriptor->Created = jTs(json, "Created");
            descriptor->Updated = jTs(json, "Updated");
            descriptor->Price = jNum(json, "PriceInRobux");

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
    case ItemType::DevProduct: {
        // A standalone dev-product backup asks the per-product endpoint; field casing differs
        // between API generations.
        std::string detailsUrl = std::vformat(Reg("internet.roblox.devproduct_details", "https://apis.roblox.com/developer-products/v1/developer-products/{}"), std::make_format_args(idStr));
        if (FetchJson(detailsUrl, "developer product " + idStr, json) && json.is_object()) {
            if (descriptor->Name.empty())
                descriptor->Name = !jStr(json, "name").empty() ? jStr(json, "name") : jStr(json, "Name");
            if (descriptor->Description.empty())
                descriptor->Description = !jStr(json, "description").empty() ? jStr(json, "description") : jStr(json, "Description");
            if (descriptor->Price == 0)
                descriptor->Price = jNum(json, "price") != 0 ? jNum(json, "price") : jNum(json, "PriceInRobux");
            if (descriptor->ImageId == 0)
                descriptor->ImageId = jNum(json, "iconImageAssetId") != 0 ? jNum(json, "iconImageAssetId") : jNum(json, "IconImageAssetId");
            MakeChild(ItemType::Asset, descriptor->ImageId);
        }
        break;
    }
    case ItemType::Outfit: {
        // Same family as the v1 avatar endpoint; it carries no owner, so the Outfit row's UserId
        // stays unset.
        std::string detailsUrl = std::vformat(Reg("internet.roblox.outfit_details", "https://avatar.roblox.com/v1/outfits/{}/details"), std::make_format_args(idStr));
        if (FetchJson(detailsUrl, "outfit " + idStr, json) && json.is_object()) {
            descriptor->Name = jStr(json, "name");
            descriptor->HasAvatar = true;

            if (auto it = json.find("assets"); it != json.end() && it->is_array()) {
                for (auto& e : *it) {
                    int64_t assetId = jNum(e, "id");
                    if (assetId <= 0)
                        continue;
                    int64_t typeId = 0;
                    if (auto t = e.find("assetType"); t != e.end() && t->is_object())
                        typeId = jNum(*t, "id");
                    descriptor->WornAssetIds.push_back(assetId);
                    MakeChild(ItemType::Asset, assetId, static_cast<Roblox::AssetType>(typeId));
                    if (mCancelled) return;
                }
            }

            if (auto scalesIt = json.find("scale"); scalesIt != json.end() && scalesIt->is_object()) {
                auto jDbl = [](const nlohmann::json& j, const char* key) -> double {
                    auto it = j.find(key);
                    return (it != j.end() && it->is_number()) ? it->get<double>() : 0.0;
                };
                descriptor->AvatarWidth       = jDbl(*scalesIt, "width");
                descriptor->AvatarHeight      = jDbl(*scalesIt, "height");
                descriptor->AvatarHead        = jDbl(*scalesIt, "head");
                descriptor->AvatarProportions = jDbl(*scalesIt, "proportion");
            }
            descriptor->AvatarBodyType = jStr(json, "playerAvatarType") == "R15" ? 1 : 0;
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
    cpr::Response res = BackupHttpGet(std::vformat(deliveryUrl, std::make_format_args(idStr)), cookie, &mCancelled);
    if (res.error.code != cpr::ErrorCode::OK || res.status_code != 200 || res.text.empty())
        return false;
    out.assign(res.text.begin(), res.text.end());
    return true;
}

// Resolves a thumbnails.roblox.com endpoint to the first Completed entry's image bytes.
static bool FetchRenderedThumbnail(const std::string& jsonUrl, std::vector<unsigned char>& out,
                                   const std::atomic<bool>* cancelled = nullptr) {
    cpr::Response res = BackupHttpGet(jsonUrl, "", cancelled);
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

    cpr::Response img = BackupHttpGet(urlIt->get<std::string>(), "", cancelled);
    if (img.error.code != cpr::ErrorCode::OK || img.status_code != 200 || img.text.empty())
        return false;

    out.assign(img.text.begin(), img.text.end());
    return true;
}

bool Backup::Process::DownloadAssetThumbnail(int64_t id, std::vector<unsigned char>& out) {
    std::string thumbTmpl = mCore->GetRegistry()->GetKeyValue<std::string>("internet.roblox.asset_thumbnail")
        .value_or("https://thumbnails.roblox.com/v1/assets?assetIds={}&size=420x420&format=Png&isCircular=false");
    std::string idStr = std::to_string(id);
    return FetchRenderedThumbnail(std::vformat(thumbTmpl, std::make_format_args(idStr)), out, &mCancelled);
}

bool Backup::Process::DownloadGameIcon(int64_t universeId, std::vector<unsigned char>& out) {
    std::string tmpl = mCore->GetRegistry()->GetKeyValue<std::string>("internet.roblox.universe_icon")
        .value_or("https://thumbnails.roblox.com/v1/games/icons?universeIds={}&size=512x512&format=Png&isCircular=false");
    std::string idStr = std::to_string(universeId);
    return FetchRenderedThumbnail(std::vformat(tmpl, std::make_format_args(idStr)), out, &mCancelled);
}

bool Backup::Process::DownloadUserThumbnail(int64_t userId, std::vector<unsigned char>& out) {
    std::string tmpl = mCore->GetRegistry()->GetKeyValue<std::string>("internet.roblox.user_thumbnail")
        .value_or("https://thumbnails.roblox.com/v1/users/avatar-headshot?userIds={}&size=420x420&format=Png&isCircular=false");
    std::string idStr = std::to_string(userId);
    return FetchRenderedThumbnail(std::vformat(tmpl, std::make_format_args(idStr)), out, &mCancelled);
}

bool Backup::Process::DownloadUserAvatar(int64_t userId, std::vector<unsigned char>& out) {
    std::string tmpl = mCore->GetRegistry()->GetKeyValue<std::string>("internet.roblox.user_avatar")
        .value_or("https://thumbnails.roblox.com/v1/users/avatar?userIds={}&size=420x420&format=Png&isCircular=false");
    std::string idStr = std::to_string(userId);
    return FetchRenderedThumbnail(std::vformat(tmpl, std::make_format_args(idStr)), out, &mCancelled);
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
    cpr::Response res = BackupHttpGet(std::vformat(tmpl, std::make_format_args(idStr)), mAuthCookie, &mCancelled);
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
        cpr::Response res = BackupHttpGet(std::vformat(tmpl, std::make_format_args(idStr)), "", &mCancelled);
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

                        cpr::Response img = BackupHttpGet(urlIt->get<std::string>(), "", &mCancelled);
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
            if (targetId == descriptor->Id) {
                db->AddPlaceholderThumbnailToPlace(descriptor->Id, blob);
                return;
            }
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
    if (mRoot->Id > 0) {
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

    const bool update = !includeId;
    const std::string name = d->Name.empty() ? std::to_string(d->Id) : d->Name;
    const char* creatorCol = d->CreatorType == Roblox::CreatorType::Group ? "GroupId" : "UserId";
    const int64_t parentUniverseId = FindParentUniverseId(d);

    switch (d->Type) {
    case ItemType::Asset:
        if (!update || !d->Name.empty())
            row.push_back({"Name", name});
        if (!update || d->AssetType != Roblox::AssetType::None)
            row.push_back({"Type", static_cast<int>(d->AssetType)});
        if (!d->Description.empty())
            row.push_back({"Description", d->Description});
        if (d->Created > 0)
            row.push_back({"Created", d->Created});
        if (d->Updated > 0)
            row.push_back({"Updated", d->Updated});
        if (d->ImageId > 0)
            row.push_back({"ImageId", d->ImageId});
        // A file-found asset never backs up its creator; don't write a dangling creator id.
        if (d->CreatorId > 0 && !d->FoundInFile)
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
        if (!update || !d->Name.empty())
            row.push_back({"Name", name});
        if (!d->Description.empty())
            row.push_back({"Description", d->Description});
        if (d->Created > 0)
            row.push_back({"Created", d->Created});
        if (d->Updated > 0)
            row.push_back({"Updated", d->Updated});
        if (d->ImageId > 0)
            row.push_back({"ImageId", d->ImageId});
        if (!update || parentUniverseId != 0)
            row.push_back({"UniverseId", parentUniverseId}); // NOT NULL; 0 = unknown
        break;
    case ItemType::User:
        row.push_back({"Name", name});
        if (!d->Description.empty()) row.push_back({"Bio", d->Description});
        if (d->HasAvatar) {
            row.push_back({"CharacterBodyType", d->AvatarBodyType});
            row.push_back({"CharacterWidth", d->AvatarWidth});
            row.push_back({"CharacterHeight", d->AvatarHeight});
            row.push_back({"CharacterHead", d->AvatarHead});
            row.push_back({"CharacterProportions", d->AvatarProportions});
        }
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
    case ItemType::Pass:
        if (!update || !d->Name.empty())
            row.push_back({"Name", name});
        if (!d->Description.empty()) row.push_back({"Description", d->Description});
        if (d->Created > 0)          row.push_back({"Created", d->Created});
        if (d->Updated > 0)          row.push_back({"Updated", d->Updated});
        if (d->ImageId > 0)          row.push_back({"ImageId", d->ImageId});
        if (d->CreatorId > 0)        row.push_back({creatorCol, d->CreatorId});
        if (!update || parentUniverseId != 0)
            row.push_back({"UniverseId", parentUniverseId});     // NOT NULL; 0 = unknown
        if (!update || d->Price != 0)
            row.push_back({"PriceInRobux", d->Price});           // NOT NULL
        break;
    case ItemType::DevProduct:
        if (!update || !d->Name.empty())
            row.push_back({"Name", name});
        if (!d->Description.empty()) row.push_back({"Description", d->Description});
        if (d->ImageId > 0)          row.push_back({"ImageId", d->ImageId});
        if (!update || parentUniverseId != 0)
            row.push_back({"UniverseId", parentUniverseId});     // NOT NULL; 0 = unknown
        if (!update || d->Price != 0)
            row.push_back({"Price", d->Price});                  // NOT NULL
        if (!update)
            row.push_back({"CurrencyType", 1});                  // NOT NULL; 1 = Robux
        break;
    case ItemType::Outfit:
        row.push_back({"Name", name}); // NOT NULL
        if (d->HasAvatar) {
            row.push_back({"BodyType", d->AvatarBodyType});
            row.push_back({"Width", d->AvatarWidth});
            row.push_back({"Height", d->AvatarHeight});
            row.push_back({"Head", d->AvatarHead});
            row.push_back({"Proportions", d->AvatarProportions});
        }
        break;
    default:
        return {};
    }
    return row;
}

void Backup::Process::DownloadItemDescriptor(Backup::ItemDescriptor* descriptor) {
    // A LocalFile root has no online id; only its children are real items.
    if (descriptor->Id <= 0)
        return;

    const std::string idStr = std::to_string(descriptor->Id);
    const std::string label = (descriptor->Name.empty() ? idStr : descriptor->Name) + " (" + idStr + ")";
    Report(State::Downloading, "Backing up " + label);

    const std::string& cookie = mAuthCookie;

    if (mOptions.DestinationType == DestinationType::Database) {
        EmuDb* db = static_cast<EmuDb*>(mOptions.Destination);

        // 1. metadata row. File-found assets defer it until the download outcome is known, so a
        // moderated/deleted reference leaves no stub row.
        bool metadataWritten = false;
        auto writeMetadataRow = [&]() {
            if (!mOptions.DownloadMetadata)
                return;
            SqlRow insertRow = BuildItemRow(descriptor, true);
            if (insertRow.empty())
                return;
            bool ok = true;
            RunDb([&]() {
                SqlDb::Response res;
                if (db->DoesItemExist(descriptor->Type, descriptor->Id)) {
                    // The update row omits placeholders (see BuildItemRow) and may then be empty.
                    SqlRow updateRow = BuildItemRow(descriptor, false);
                    res = updateRow.empty() ? SqlDb::Response::Success
                                            : db->UpdateItem(descriptor->Type, descriptor->Id, updateRow);
                } else {
                    res = db->AddItem(descriptor->Type, insertRow);
                }
                if (res == SqlDb::Response::Success) db->MarkDirty();
                else ok = false;
            });
            if (!ok) {
                mFailedWrites++;
                Report(State::AddingToDatabaseFailed, "Failed to write metadata for " + label);
                return;
            }
            metadataWritten = true;
        };
        const bool deferMetadata = descriptor->Type == ItemType::Asset && descriptor->FoundInFile;
        if (!deferMetadata)
            writeMetadataRow();

        // 2. only assets carry downloadable binary content and thumbnails
        if (descriptor->Type == ItemType::Asset) {
            std::vector<unsigned char> data;
            const bool dataOk = DownloadAssetData(descriptor->Id, cookie, data) && !data.empty();

            // Only materialize a file-found row that verifiably exists (bytes downloaded or enrichment
            // recognized the id).
            if (deferMetadata && !mCancelled) {
                const bool enriched = !descriptor->Name.empty() || descriptor->AssetType != Roblox::AssetType::None;
                if (dataOk || enriched)
                    writeMetadataRow();
            }

            if (dataOk) {
                bool ok = true;
                RunDb([&]() {
                    if (db->AttachDataToAsset(descriptor->Id, 0, data) == SqlDb::Response::Success) db->MarkDirty();
                    else ok = false;
                });
                if (!ok) {
                    mFailedWrites++;
                    Report(State::AddingToDatabaseFailed, "Failed to store data for " + label);
                }
                if (mOptions.ParseFilesAndBackupFoundAssets && !mCancelled)
                    ParseFileForAssetReferences(descriptor, data);
            } else if (!mCancelled) {
                mFailedDownloads++;
                Report(State::DownloadingFailed, "Could not download data for " + label);
            }

            // A file-found place is a referenced asset, not one of the target's own places, no universe
            // link or carousel.
            if (descriptor->AssetType == Roblox::AssetType::Place && !descriptor->FoundInFile) {
                int64_t universeId = FindParentUniverseId(descriptor);
                if (universeId != 0)
                    RunDb([&]() { db->AddPlaceToUniverse(universeId, descriptor->Id); });
            }

            // imageId 0 = Roblox's placeholder render -> AutogeneratedThumbnailHash. File-found assets use
            // the enrichment URL (per-asset fetch when the CDN link went stale).
            if (mOptions.DownloadAutoGeneratedThumbnails && !mCancelled && descriptor->ImageId == 0 &&
                (!deferMetadata || metadataWritten)) {
                std::vector<unsigned char> thumb;
                bool got = false;
                if (descriptor->FoundInFile) {
                    if (!descriptor->ThumbnailUrl.empty()) {
                        cpr::Response img = BackupHttpGet(descriptor->ThumbnailUrl, "", &mCancelled);
                        if (img.error.code == cpr::ErrorCode::OK && img.status_code == 200 && !img.text.empty()) {
                            thumb.assign(img.text.begin(), img.text.end());
                            got = true;
                        }
                        if (!got && !mCancelled)
                            got = DownloadAssetThumbnail(descriptor->Id, thumb);
                    }
                } else {
                    got = DownloadAssetThumbnail(descriptor->Id, thumb);
                }
                if (got && !thumb.empty()) {
                    RunDb([&]() {
                        if (db->AttachThumbnailDataToAsset(descriptor->Id, thumb) == SqlDb::Response::Success)
                            db->MarkDirty();
                    });
                }
            }

            if (descriptor->AssetType == Roblox::AssetType::Place && !descriptor->FoundInFile &&
                mOptions.DownloadAutoGeneratedThumbnails && !mCancelled)
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

        // 4. a user's worn avatar: link worn assets to the character and store body colors.
        if (descriptor->Type == ItemType::User && descriptor->HasAvatar && mOptions.DownloadMetadata && !mCancelled) {
            RunDb([&]() {
                for (int64_t assetId : descriptor->WornAssetIds)
                    db->AddAssetToUserCharacter(descriptor->Id, assetId);
                for (const auto& [part, color] : descriptor->BodyColors)
                    db->SetUserCharacterBodyColor(descriptor->Id, part, color);
                db->MarkDirty();
            });
        }

        // 5. an outfit's contents: link each worn asset to the outfit.
        if (descriptor->Type == ItemType::Outfit && descriptor->HasAvatar && mOptions.DownloadMetadata && !mCancelled) {
            RunDb([&]() {
                for (int64_t assetId : descriptor->WornAssetIds)
                    db->AddAssetToOutfit(descriptor->Id, assetId);
                db->MarkDirty();
            });
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
                    mFailedWrites++;
                    Report(State::AddingToDatabaseFailed, "Could not write file for " + label);
                }
                if (mOptions.ParseFilesAndBackupFoundAssets && !mCancelled)
                    ParseFileForAssetReferences(descriptor, data);
            } else if (!mCancelled) {
                mFailedDownloads++;
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

static void CollectAssetIdsFromText(std::string_view text, std::vector<int64_t>& out) {
    auto digitsAt = [](std::string_view s, size_t pos) -> int64_t {
        int64_t value = 0;
        bool any = false;
        while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) {
            value = value * 10 + (s[pos] - '0');
            any = true;
            pos++;
            if (value > 999999999999999LL) return 0; // absurdly long digit run: not an asset id
        }
        return any ? value : 0;
    };

    constexpr std::string_view kScheme = "rbxassetid://";
    for (size_t pos = text.find(kScheme); pos != std::string_view::npos; pos = text.find(kScheme, pos + 1)) {
        int64_t id = digitsAt(text, pos + kScheme.size());
        if (id > 0) out.push_back(id);
    }

    // The URL starting at pos, ended at the first character a URL cannot contain.
    auto urlAt = [&](size_t pos) -> std::string_view {
        size_t end = pos;
        while (end < text.size()) {
            unsigned char c = static_cast<unsigned char>(text[end]);
            if (c <= ' ' || c == '"' || c == '\'' || c == '<' || c == '>' || c == ')' || c == '\\' || c >= 0x7F)
                break;
            end++;
        }
        return text.substr(pos, end - pos);
    };
    auto collectIdParam = [&](std::string_view url) {
        constexpr std::string_view kIdParam = "id=";
        for (size_t pos = url.find(kIdParam); pos != std::string_view::npos; pos = url.find(kIdParam, pos + 1)) {
            // Require ?id= or &id= so userId=/universeId=/placeId= and friends don't match.
            if (pos == 0 || (url[pos - 1] != '?' && url[pos - 1] != '&'))
                continue;
            int64_t id = digitsAt(url, pos + kIdParam.size());
            if (id > 0) out.push_back(id);
        }
    };

    constexpr std::string_view kThumb = "rbxthumb://";
    for (size_t pos = text.find(kThumb); pos != std::string_view::npos; pos = text.find(kThumb, pos + 1)) {
        std::string_view url = urlAt(pos);
        if (url.find("type=Asset") != std::string_view::npos)
            collectIdParam(url);
    }

    constexpr std::string_view kDomain = "roblox.com";
    for (size_t pos = text.find(kDomain); pos != std::string_view::npos; pos = text.find(kDomain, pos + 1))
        collectIdParam(urlAt(pos));
}

bool Backup::Process::ParseFileForAssetReferences(Backup::ItemDescriptor* descriptor, const std::vector<unsigned char>& data) {
    constexpr std::string_view kMagic = "<roblox";
    std::string_view head(reinterpret_cast<const char*>(data.data()), std::min<size_t>(data.size(), 256));
    bool looksLikeRobloxFile = head.starts_with(kMagic);
    if (!looksLikeRobloxFile) {
        size_t offset = head.starts_with("\xef\xbb\xbf") ? 3 : 0;
        while (offset < head.size() &&
               (head[offset] == ' ' || head[offset] == '\t' || head[offset] == '\r' || head[offset] == '\n'))
            offset++;
        looksLikeRobloxFile = offset < head.size() && head[offset] == '<' &&
                              head.find(kMagic) != std::string_view::npos;
    }
    if (!looksLikeRobloxFile)
        return false;

    const std::string idStr = std::to_string(descriptor->Id);
    const std::string label = descriptor->Name.empty() ? idStr : descriptor->Name;
    Report(State::ParsingFile, "Scanning " + label + " for asset references");

    // A throw escaping the worker thread would std::terminate; parse failures stay contained here.
    std::vector<int64_t> ids;
    try {
        std::unique_ptr<Roblox::RobloxFile> file;
        if (Roblox::RobloxFile::Open(file, data) != Roblox::FileResponse::Success || file == nullptr) {
            Report(State::ParsingFileFailed, "Could not parse the file of " + label);
            return false;
        }

        for (Roblox::Instance* inst : file->GetDescendants()) {
            if (mCancelled)
                return true;
            for (const auto& [name, prop] : inst->GetProperties()) {
                if (const auto* str = prop.CastValue<std::string>())
                    CollectAssetIdsFromText(*str, ids);
                else if (const auto* contentId = prop.CastValue<Roblox::DataTypes::ContentId>())
                    CollectAssetIdsFromText(contentId->Uri, ids);
                else if (const auto* content = prop.CastValue<Roblox::DataTypes::Content>()) {
                    if (content->SourceType == Roblox::DataTypes::ContentSourceType::Uri)
                        CollectAssetIdsFromText(content->Uri, ids);
                }
                // ProtectedString sources and SharedString blobs regularly embed asset references; scan them too.
                else if (const auto* script = prop.CastValue<Roblox::DataTypes::ProtectedString>())
                    CollectAssetIdsFromText(std::string_view(
                        reinterpret_cast<const char*>(script->RawBuffer.data()), script->RawBuffer.size()), ids);
                else if (const auto* shared = prop.CastValue<Roblox::DataTypes::SharedString>())
                    CollectAssetIdsFromText(std::string_view(
                        reinterpret_cast<const char*>(shared->Value.data()), shared->Value.size()), ids);
            }
        }
    } catch (...) {
        Report(State::ParsingFileFailed, "Could not parse the file of " + label);
        return false;
    }

    int added = 0;
    std::vector<ItemDescriptor*> newChildren;
    RunDb([&]() {
        for (int64_t id : ids) {
            if (mCancelled)
                return;
            if (id <= 0 || mDiscoveredItems.contains({ ItemType::Asset, id }))
                continue;
            if (mDiscoveredItems.size() >= kMaxBackupNodes) {
                Report(State::ParsingFile, "Reached the backup item limit (" + std::to_string(kMaxBackupNodes) +
                                           "); skipping the remaining file references.");
                break;
            }
            mDiscoveredItems.insert({ ItemType::Asset, id });
            auto* child = new ItemDescriptor();
            child->Type = ItemType::Asset;
            child->Id = id;
            child->FoundInFile = true;
            descriptor->AddChild(child);
            newChildren.push_back(child);
            mTotalNodes++;
            added++;
        }
    });
    if (added > 0)
        Report(State::ParsingFile, "Found " + std::to_string(added) + " referenced assets in " + label);
    if (!newChildren.empty() && !mCancelled)
        EnrichFoundAssets(newChildren);
    return true;
}

void Backup::Process::EnrichFoundAssets(const std::vector<ItemDescriptor*>& items) {
    // Runs even without DownloadMetadata: it is the file-found assets' only source of previews.
    if (items.empty() || (!mOptions.DownloadMetadata && !mOptions.DownloadAutoGeneratedThumbnails))
        return;

    auto Reg = [&](const char* key, const char* fallback) -> std::string {
        return mCore->GetRegistry()->GetKeyValue<std::string>(key).value_or(fallback);
    };

    // The per-asset economy endpoint allows roughly one request a minute; develop.roblox.com
    // serves 50 ids per call and the thumbnails endpoint takes the same list.
    constexpr size_t kBatch = 50;
    size_t matched = 0; // ids the develop endpoint actually returned metadata for
    for (size_t start = 0; start < items.size() && !mCancelled; start += kBatch) {
        const size_t count = std::min(kBatch, items.size() - start);
        std::string idList;
        std::map<int64_t, ItemDescriptor*> byId;
        for (size_t i = start; i < start + count; i++) {
            if (!idList.empty()) idList += ",";
            idList += std::to_string(items[i]->Id);
            byId[items[i]->Id] = items[i];
        }

        if (!mOptions.DownloadMetadata) {
            // Thumbnails-only run; skip the develop call entirely.
        } else if (mAuthCookie.empty()) {
            if (!mWarnedNoAuthForEnrich) {
                mWarnedNoAuthForEnrich = true;
                Report(State::ParsingFile, "Not logged into a Roblox account; file-found assets keep "
                                           "placeholder names (thumbnails still work).");
            }
        } else {
            std::string metaUrl = std::vformat(Reg("internet.roblox.asset_batch_details",
                "https://develop.roblox.com/v1/assets?assetIds={}"), std::make_format_args(idList));
            cpr::Response res = BackupHttpGet(metaUrl, mAuthCookie, &mCancelled);
            if (res.error.code == cpr::ErrorCode::OK && res.status_code == 200) {
                nlohmann::json root = nlohmann::json::parse(res.text, nullptr, false);
                if (!root.is_discarded() && root.contains("data") && root["data"].is_array()) {
                    // Field writes go through the UI thread like the appends did.
                    RunDb([&]() {
                        for (auto& a : root["data"]) {
                            auto idIt = a.find("id");
                            if (idIt == a.end() || !idIt->is_number())
                                continue;
                            auto found = byId.find(idIt->get<int64_t>());
                            if (found == byId.end())
                                continue;
                            matched++;
                            ItemDescriptor* d = found->second;
                            if (auto it = a.find("name"); it != a.end() && it->is_string())
                                d->Name = it->get<std::string>();
                            if (auto it = a.find("description"); it != a.end() && it->is_string())
                                d->Description = it->get<std::string>();
                            if (auto it = a.find("created"); it != a.end() && it->is_string()) {
                                int64_t ts = ParseIso8601Utc(it->get<std::string>());
                                if (ts > 0) d->Created = ts;
                            }
                            if (auto it = a.find("updated"); it != a.end() && it->is_string()) {
                                int64_t ts = ParseIso8601Utc(it->get<std::string>());
                                if (ts > 0) d->Updated = ts;
                            }
                            if (auto it = a.find("type"); it != a.end() && it->is_string() &&
                                d->AssetType == Roblox::AssetType::None)
                                d->AssetType = AssetTypeFromApiString(it->get<std::string>());
                            if (auto it = a.find("creator"); it != a.end() && it->is_object()) {
                                int64_t creatorId = 0;
                                if (auto t = it->find("targetId"); t != it->end() && t->is_number())
                                    creatorId = t->get<int64_t>();
                                if (creatorId == 0)
                                    if (auto t = it->find("id"); t != it->end() && t->is_number())
                                        creatorId = t->get<int64_t>();
                                std::string creatorType;
                                if (auto t = it->find("type"); t != it->end() && t->is_string())
                                    creatorType = t->get<std::string>();
                                if (creatorId > 0) {
                                    d->CreatorId = creatorId;
                                    d->CreatorType = creatorType == "Group" ? Roblox::CreatorType::Group
                                                                            : Roblox::CreatorType::User;
                                }
                            }
                        }
                    });
                }
            } else {
                // Count once per failed batch so a rate-limited enrichment shows in the run summary.
                mFailedDownloads++;
                if (res.status_code == 429) {
                    mRatelimitedHits++;
                    Report(State::Ratelimited, "HTTP 429 while fetching metadata for file-found assets");
                } else {
                    Report(State::DownloadingFailed, "Could not fetch metadata for " + std::to_string(count) +
                                                     " file-found assets (HTTP " + std::to_string(res.status_code) + ")");
                }
            }
        }
        if (mCancelled)
            return;

        // Resolve thumbnail URLs now (one batch); the images download with their asset in phase 2.
        if (mOptions.DownloadAutoGeneratedThumbnails) {
            std::string thumbUrl = std::vformat(Reg("internet.roblox.asset_thumbnail",
                "https://thumbnails.roblox.com/v1/assets?assetIds={}&size=420x420&format=Png&isCircular=false"),
                std::make_format_args(idList));
            cpr::Response res = BackupHttpGet(thumbUrl, "", &mCancelled);
            if (res.status_code == 429)
                mRatelimitedHits++;
            if (res.error.code == cpr::ErrorCode::OK && res.status_code == 200) {
                nlohmann::json root = nlohmann::json::parse(res.text, nullptr, false);
                if (!root.is_discarded() && root.contains("data") && root["data"].is_array()) {
                    RunDb([&]() {
                        for (auto& t : root["data"]) {
                            auto stateIt = t.find("state");
                            auto urlIt   = t.find("imageUrl");
                            auto tidIt   = t.find("targetId");
                            if (stateIt == t.end() || !stateIt->is_string() || stateIt->get<std::string>() != "Completed")
                                continue;
                            if (urlIt == t.end() || !urlIt->is_string() || tidIt == t.end() || !tidIt->is_number())
                                continue;
                            auto found = byId.find(tidIt->get<int64_t>());
                            if (found != byId.end())
                                found->second->ThumbnailUrl = urlIt->get<std::string>();
                        }
                    });
                }
            }
        }

        // Stay comfortably under develop.roblox.com's per-minute budget between batches.
        if (start + kBatch < items.size())
            for (int slept = 0; slept < 700 && !mCancelled; slept += 50)
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // "X of N": the develop endpoint omits deleted/moderated ids, so the two routinely differ.
    if (mOptions.DownloadMetadata && !mAuthCookie.empty())
        Report(State::ParsingFile, "Fetched metadata for " + std::to_string(matched) + " of " +
                                   std::to_string(items.size()) + " referenced assets");
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
