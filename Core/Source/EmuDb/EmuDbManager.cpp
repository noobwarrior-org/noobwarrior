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
// File: DatabaseManager.cpp
// Started by: Hattozo
// Started on: 3/17/2025
// Description: Loads in multiple databases with different priorities over one another.
// This is used in situations where you want to have multiple databases loaded at the same time for different reasons,
// but these databases may have conflicting IDs in them. In this case, a system to manage priority is required.
//
// This also handles authentication, but will outsource it to a master server if set.
#include "NoobWarrior/EmuDb/ContentImages.h"
#include <NoobWarrior/EmuDb/EmuDbManager.h>
#include <NoobWarrior/EmuDb/EmuDb.h>
#include <NoobWarrior/PluginManager.h>
#include <NoobWarrior/Registry.h>
#include <NoobWarrior/Url.h>
#include <NoobWarrior/NoobWarrior.h>

#include <sol/sol.hpp>

#include <algorithm>
#include <format>
#include <unordered_set>
#include <vector>

using namespace NoobWarrior;

EmuDbManager::EmuDbManager(Core *core) :
    mCore(core)
{}

void EmuDbManager::MountDatabases() {
    auto mounted = mCore->GetRegistry()->GetKeyValue<sol::table>("databases.mounted");
    if (!mounted.has_value())
        return;

    int filePriority = 0;
    for (int i = 1; i <= (int)(*mounted).size(); i++) {
        sol::object val = (*mounted)[i];
        if (val.get_type() == sol::type::lua_nil)
            break;
        if (!val.is<std::string>())
            continue;
        std::string entry = val.as<std::string>();

        // An entry carrying a protocol is a database a plugin offered and the user opted into. It is
        // stored as a URL rather than a path because the file it resolves to may be a staged copy,
        // and because the plugin can move between the install and user directories.
        if (Url(entry).DoesStringHaveProtocol()) {
            if (mCore->GetPluginManager()->MountDeclaredDatabase(entry, filePriority))
                filePriority++;
            continue;
        }

        if (!std::filesystem::exists(entry))
            continue;
        if (Mount(std::filesystem::path(entry), filePriority) == SqlDb::FailReason::None)
            filePriority++;
    }
}

void EmuDbManager::UnmountDatabases() {
    for (auto *db : mMountedDatabases) {
        Out("EmuDbManager", "Unmounted database \"{}\"", db->GetFileName());
        NOOBWARRIOR_FREE_PTR(db)
    }
    mMountedDatabases.clear();
    mMountInfo.clear();
    ClearTemporaryDatabase(); // the scratch db's materialized items are meaningless without the mount set
}

EmuDb* EmuDbManager::GetTemporaryDatabase() {
    if (mTemporaryDatabase == nullptr) {
        // Ephemeral in-memory database (vanishes on close). Migrations run in the ctor, so it's schema-ready.
        mTemporaryDatabase = new EmuDb(":memory:", true);
        if (mTemporaryDatabase->Fail()) {
            Out("EmuDbManager", "Failed to create the temporary in-memory database");
            NOOBWARRIOR_FREE_PTR(mTemporaryDatabase)
        }
    }
    return mTemporaryDatabase;
}

void EmuDbManager::ClearTemporaryDatabase() {
    if (mTemporaryDatabase != nullptr)
        NOOBWARRIOR_FREE_PTR(mTemporaryDatabase) // freeing the :memory: db reclaims its RAM; recreated on demand
    mMaterializedIds.clear();
    mNextSynthId = (1LL << 48);
}

int64_t EmuDbManager::MaterializeAsset(const std::string &originKey, int assetType, const std::string &name,
                                       const std::vector<unsigned char> &assetData) {
    // Idempotent: the same source item always maps to the same synthetic id.
    // WTF is an "Idempotent"?
    if (auto it = mMaterializedIds.find(originKey); it != mMaterializedIds.end())
        return it->second;

    EmuDb *temp = GetTemporaryDatabase();
    if (temp == nullptr)
        return 0;

    const int64_t synthId = mNextSynthId++;
    SqlRow row;
    row.push_back({ "Id", synthId });
    row.push_back({ "Type", assetType });
    row.push_back({ "Name", name });
    if (temp->AddItem(ItemType::Asset, row) != SqlDb::Response::Success)
        return 0;
    temp->AttachDataToAsset(synthId, 1, assetData); // version 1; RetrieveAssetData(id, 0) returns it as latest

    mMaterializedIds[originKey] = synthId;
    return synthId;
}

std::optional<int64_t> EmuDbManager::GetMaterializedId(const std::string &originKey) const {
    if (auto it = mMaterializedIds.find(originKey); it != mMaterializedIds.end())
        return it->second;
    return std::nullopt;
}

bool EmuDbManager::CacheAssetInTemporary(int64_t id, const std::vector<unsigned char> &data) {
    EmuDb *temp = GetTemporaryDatabase();
    if (temp == nullptr)
        return false;
    if (temp->DoesItemExist(ItemType::Asset, id))
        return true; // already cached this id (only reached on a full miss, so this is just a guard)

    SqlRow row;
    row.push_back({ "Id", id });
    row.push_back({ "Type", 0 });          // type is irrelevant for serving the blob back
    row.push_back({ "Name", std::string() });
    if (temp->AddItem(ItemType::Asset, row) != SqlDb::Response::Success)
        return false;
    return temp->AttachDataToAsset(id, 1, data) == SqlDb::Response::Success;
}

SqlDb::Response EmuDbManager::MountMasterDbIfNotAlreadyMounted() {
    if (GetMasterDatabase() == nullptr) {
        Out("EmuDbManager", "No master database found. This is because the list of selected databases in the registry is empty. Automatically mounting one for you...");
        std::filesystem::path absolutePath = mCore->GetUserDataDir() / "databases" / "master.nwdb";
        if (!std::filesystem::exists(absolutePath))
            Out("EmuDbManager", "Creating master database named \"master.nwdb\"...");
        else
            Out("EmuDbManager", "File named \"master.nwdb\" already exists, using that as the master database instead...");

        auto *db = new EmuDb(absolutePath.string(), true);
        if (db->Fail()) {
            Out("EmuDbManager", "Failed to create master database");
            return SqlDb::Response::DatabaseFailed;
        }
        db->SetTitle("Master Database");
        db->SetDescription("This is the default database created by noobWarrior.\nThis will only be the primary database if it is the highest on the list.");
        bool res = Mount(db, 0);
        return res ? SqlDb::Response::Success : SqlDb::Response::Failed;
    }
    return SqlDb::Response::DidNothing;
}

SqlDb::FailReason EmuDbManager::MountOwned(const std::filesystem::path &filePath, unsigned int priority,
                                           const MountInfo &info, SqlDb::OpenMode openMode) {
    auto *database = new EmuDb(filePath.string(), true, openMode);
    if (database->Fail()) {
        SqlDb::FailReason reason = database->GetFailReason();
        NOOBWARRIOR_FREE_PTR(database)
        return reason;
    }
    mMountedDatabases.insert(
        mMountedDatabases.begin() + std::min(static_cast<size_t>(priority), mMountedDatabases.size()),
        database);
    mMountInfo[database] = info;
    Out("EmuDbManager", "Mounted database \"{}\"{}{}", database->GetFileName(),
        database->IsReadOnly() ? " (read-only)" : "",
        info.OwnerPluginId.empty() ? "" : std::format(" for plugin \"{}\"", info.OwnerPluginId));
    return SqlDb::FailReason::None;
}

// A database whose Mutable flag is off asks not to be written to at runtime, so honor that at the
// SQLite level rather than trusting every write path to check. The flag lives inside the file, so it
// has to be probed on a separate connection before deciding how to open it for real.
static SqlDb::OpenMode OpenModeForUserMount(const std::filesystem::path &filePath) {
    return EmuDb::ProbeIsMutable(filePath) ? SqlDb::OpenMode::ReadWrite : SqlDb::OpenMode::ReadOnly;
}

SqlDb::FailReason EmuDbManager::Mount(const std::filesystem::path &filePath, unsigned int priority) {
    return MountOwned(filePath, priority, MountInfo {}, OpenModeForUserMount(filePath));
}

SqlDb::FailReason EmuDbManager::Mount(const std::string &dbName, unsigned int priority) {
    std::filesystem::path absolutePath = mCore->GetUserDataDir() / NW_PATH_DATABASES / (dbName + ".nwdb");
    return MountOwned(absolutePath, priority, MountInfo {}, OpenModeForUserMount(absolutePath));
}

bool EmuDbManager::Mount(EmuDb* database, unsigned int priority) {
    if (database->Fail()) return false;
    if (std::find(mMountedDatabases.begin(), mMountedDatabases.end(), database) != mMountedDatabases.end())
        return false;
    mMountedDatabases.insert(
        mMountedDatabases.begin() + std::min(static_cast<size_t>(priority), mMountedDatabases.size()),
        database);
    mMountInfo[database] = MountInfo {};
    Out("EmuDbManager", "Mounted database \"{}\"", database->GetFileName());
    return true;
}

bool EmuDbManager::Unmount(EmuDb* database) {
    auto it = std::find(mMountedDatabases.begin(), mMountedDatabases.end(), database);
    if (it == mMountedDatabases.end())
        return false;
    mMountedDatabases.erase(it);
    mMountInfo.erase(database);
    Out("EmuDbManager", "Unmounted database \"{}\"", database->GetFileName());
    return true;
}

const EmuDbManager::MountInfo* EmuDbManager::GetMountInfo(EmuDb* database) const {
    auto it = mMountInfo.find(database);
    return it != mMountInfo.end() ? &it->second : nullptr;
}

bool EmuDbManager::IsLocked(EmuDb* database) const {
    const MountInfo *info = GetMountInfo(database);
    return info != nullptr && info->Locked;
}

std::vector<EmuDb*> EmuDbManager::GetUserMountedDatabases() {
    std::vector<EmuDb*> databases;
    for (auto *db : mMountedDatabases) {
        const MountInfo *info = GetMountInfo(db);
        if (info == nullptr || info->OwnerPluginId.empty())
            databases.push_back(db);
    }
    return databases;
}

EmuDb* EmuDbManager::GetDbFromSourceUrl(const std::string &sourceUrl) {
    if (sourceUrl.empty())
        return nullptr;
    for (auto *db : mMountedDatabases) {
        const MountInfo *info = GetMountInfo(db);
        if (info != nullptr && info->SourceUrl == sourceUrl)
            return db;
    }
    return nullptr;
}

void EmuDbManager::UnmountDatabasesForPlugin(const std::string &identifier) {
    // Collect first: Unmount() erases from the vector we would otherwise be iterating.
    std::vector<EmuDb*> owned;
    for (auto *db : mMountedDatabases) {
        const MountInfo *info = GetMountInfo(db);
        if (info != nullptr && info->OwnerPluginId == identifier)
            owned.push_back(db);
    }
    for (auto *db : owned) {
        Unmount(db);
        NOOBWARRIOR_FREE_PTR(db)
    }
}

EmuDb *EmuDbManager::GetMasterDatabase() {
    return mMountedDatabases.size() > 0 ? mMountedDatabases.at(0) : nullptr;
}

std::vector<EmuDb*> EmuDbManager::GetMountedDatabases() {
    return mMountedDatabases;
}

void EmuDbManager::SetMountOrder(const std::vector<EmuDb*>& order) {
    mMountedDatabases = order;
    // Anything the caller dropped is no longer mounted, and a stale entry would make
    // GetMountInfo() answer for a database that is not in the list.
    std::erase_if(mMountInfo, [&order](const auto &entry) {
        return std::find(order.begin(), order.end(), entry.first) == order.end();
    });
}

EmuDb* EmuDbManager::GetDbFromFilePath(const std::filesystem::path &path) {
    for (auto *db : mMountedDatabases) {
        std::error_code ec;
        if (std::filesystem::equivalent(path, db->GetFilePath(), ec) && !ec)
            return db;
    }
    return nullptr;
}

EmuDb* EmuDbManager::GetDbFromFileName(const std::string &name) {
    for (auto *db : mMountedDatabases) {
        if (db->GetFileName() == (name.ends_with(".nwdb") ? name : name + ".nwdb"))
            return db;
    }
    return nullptr;
}

EmuDb* EmuDbManager::GetFirstDbWhereItemExists(ItemType type, int64_t id) {
    for (EmuDb* db : mMountedDatabases) {
        bool exists = db->DoesItemExist(type, id);
        if (exists)
            return db;
    }
    // Lowest-priority fallback: a materialized transient item lives only in the scratch database.
    if (mTemporaryDatabase != nullptr && mTemporaryDatabase->DoesItemExist(type, id))
        return mTemporaryDatabase;
    return nullptr;
}

SqlDb::Response EmuDbManager::RetrieveAssetDataHash(int64_t id, int version, std::string *hashOutput) {
    for (EmuDb* db : mMountedDatabases) {
        SqlDb::Response res = db->RetrieveAssetDataHash(id, version, hashOutput);
        if (res == SqlDb::Response::Success)
            return res;
    }
    if (mTemporaryDatabase != nullptr) {
        SqlDb::Response res = mTemporaryDatabase->RetrieveAssetDataHash(id, version, hashOutput);
        if (res == SqlDb::Response::Success)
            return res;
    }
    return SqlDb::Response::NotFound;
}

EmuDb* EmuDbManager::GetWritableDbForItem(ItemType type, int64_t id) {
    EmuDb *owner = GetFirstDbWhereItemExists(type, id);
    if (owner != nullptr && owner->AllowsRuntimeWrites())
        return owner;

    EmuDb *master = GetMasterDatabase();
    if (master != nullptr && !master->AllowsRuntimeWrites())
        return nullptr;

    if (owner != nullptr && master != nullptr && owner != master) {
        Out("EmuDbManager", "Database \"{}\" does not accept runtime writes; writing to \"{}\" instead",
            owner->GetFileName(), master->GetFileName());
    }
    return master;
}

SqlDb::Response EmuDbManager::RetrieveAssetData(int64_t id, int version, std::vector<unsigned char> *dataOutput, std::string *hashOutput) {
    for (EmuDb* db : mMountedDatabases) {
        SqlDb::Response res = db->RetrieveAssetData(id, version, dataOutput, hashOutput);
        if (res == SqlDb::Response::Success)
            return res;
    }
    if (mTemporaryDatabase != nullptr) {
        SqlDb::Response res = mTemporaryDatabase->RetrieveAssetData(id, version, dataOutput, hashOutput);
        if (res == SqlDb::Response::Success)
            return res;
    }
    return SqlDb::Response::NotFound;
}

std::optional<int64_t> EmuDbManager::GetUniverseIdForPlace(int64_t placeId) {
    for (EmuDb* db : mMountedDatabases) {
        if (auto universeId = db->GetUniverseIdForPlace(placeId))
            return universeId;
    }
    return std::nullopt;
}

std::optional<int64_t> EmuDbManager::GetStartPlaceIdForUniverse(int64_t universeId) {
    for (EmuDb* db : mMountedDatabases) {
        if (auto placeId = db->GetStartPlaceIdForUniverse(universeId))
            return placeId;
    }
    return std::nullopt;
}

std::optional<int> EmuDbManager::GetUniverseAvatarType(int64_t universeId) {
    for (EmuDb* db : mMountedDatabases) {
        if (auto avatarType = db->GetUniverseAvatarType(universeId))
            return avatarType;
    }
    return std::nullopt;
}

std::optional<bool> EmuDbManager::GetUniverseVoiceChatEnabled(int64_t universeId) {
    for (EmuDb* db : mMountedDatabases) {
        if (auto enabled = db->GetUniverseVoiceChatEnabled(universeId))
            return enabled;
    }
    return std::nullopt;
}

std::optional<std::string> EmuDbManager::GetItemName(ItemType type, int64_t id) {
    for (EmuDb* db : mMountedDatabases) {
        if (auto name = db->GetItemName(type, id))
            return name;
    }
    return std::nullopt;
}

std::optional<int64_t> EmuDbManager::GetCreatorUserId(ItemType type, int64_t id) {
    for (EmuDb* db : mMountedDatabases) {
        if (auto userId = db->GetCreatorUserId(type, id))
            return userId;
    }
    return std::nullopt;
}

std::vector<int64_t> EmuDbManager::SearchAssetIds(Roblox::AssetType type, const std::string &keyword, int limit, int offset) {
    if (limit <= 0) limit = 30;
    if (offset < 0) offset = 0;

    // Pull enough from each database to satisfy offset+limit, then merge in mount-priority order
    // while dropping ids already seen in a higher-priority database.
    std::vector<int64_t> merged;
    std::unordered_set<int64_t> seen;
    for (EmuDb* db : mMountedDatabases) {
        for (int64_t id : db->SearchAssetIds(type, keyword, limit + offset, 0)) {
            if (seen.insert(id).second)
                merged.push_back(id);
        }
    }

    std::vector<int64_t> out;
    for (size_t i = static_cast<size_t>(offset); i < merged.size() && out.size() < static_cast<size_t>(limit); i++)
        out.push_back(merged[i]);
    return out;
}

std::optional<EmuDb::AssetSummary> EmuDbManager::GetAssetSummary(int64_t id) {
    for (EmuDb* db : mMountedDatabases) {
        if (auto summary = db->GetAssetSummary(id))
            return summary;
    }
    return std::nullopt;
}

std::optional<EmuDb::AssetProductInfo> EmuDbManager::GetAssetProductInfo(int64_t id) {
    for (EmuDb* db : mMountedDatabases) {
        if (auto info = db->GetAssetProductInfo(id))
            return info;
    }
    return std::nullopt;
}

std::vector<int64_t> EmuDbManager::ListUniverseIds(bool groupOwned, int limit, int offset) {
    if (limit <= 0) limit = 50;
    if (offset < 0) offset = 0;

    // Pull enough from each database to satisfy offset+limit, then merge in mount-priority order
    // while dropping ids already seen in a higher-priority database.
    std::vector<int64_t> merged;
    std::unordered_set<int64_t> seen;
    for (EmuDb* db : mMountedDatabases) {
        for (int64_t id : db->ListUniverseIds(groupOwned, limit + offset, 0)) {
            if (seen.insert(id).second)
                merged.push_back(id);
        }
    }

    std::vector<int64_t> out;
    for (size_t i = static_cast<size_t>(offset); i < merged.size() && out.size() < static_cast<size_t>(limit); i++)
        out.push_back(merged[i]);
    return out;
}

std::optional<EmuDb::UniverseSummary> EmuDbManager::GetUniverseSummary(int64_t id) {
    for (EmuDb* db : mMountedDatabases) {
        if (auto summary = db->GetUniverseSummary(id))
            return summary;
    }
    return std::nullopt;
}

std::vector<int64_t> EmuDbManager::ListUniversePlaceIds(int64_t universeId) {
    std::vector<int64_t> merged;
    std::unordered_set<int64_t> seen;
    for (EmuDb* db : mMountedDatabases) {
        for (int64_t placeId : db->ListUniversePlaceIds(universeId)) {
            if (seen.insert(placeId).second)
                merged.push_back(placeId);
        }
    }
    return merged;
}

std::vector<unsigned char> EmuDbManager::RetrieveImageData(ItemType type, int64_t id) {
    if (EmuDb* db = GetFirstDbWhereItemExists(type, id))
        return db->RetrieveImageData(type, id);
    // No database actually has the item; let the highest-priority one return its placeholder icon.
    if (!mMountedDatabases.empty())
        return mMountedDatabases.front()->RetrieveImageData(type, id);
    return {};
}

std::vector<unsigned char> EmuDbManager::RetrievePlaceThumbnailData(int64_t placeId) {
    // An overlay database can contain the place metadata without having backed up its carousel.
    // Continue through lower-priority mounts until one supplies the actual thumbnail bytes.
    for (EmuDb* db : mMountedDatabases) {
        std::vector<unsigned char> image = db->RetrievePlaceThumbnailData(placeId);
        if (!image.empty())
            return image;
    }
    return {};
}
