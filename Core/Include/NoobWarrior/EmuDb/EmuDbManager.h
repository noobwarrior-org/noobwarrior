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
// File: DatabaseManager.h
// Started by: Hattozo
// Started on: 3/17/2025
// Description:
#pragma once
#include <NoobWarrior/EmuDb/EmuDb.h>

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace NoobWarrior {
class Core;
class EmuDbManager {
public:
    EmuDbManager(Core *core);

    void MountDatabases();

    // NOTE: THIS DELETES ALL THE MOUNTED DATABASES FROM MEMORY!!!!
    void UnmountDatabases();

    // CALL MOUNTDATABASES() BEFORE WASTING YOUR TIME ON THIS FUNCTION!!!
    SqlDb::Response MountMasterDbIfNotAlreadyMounted();

    SqlDb::FailReason Mount(const std::filesystem::path &filePath, unsigned int priority);
    SqlDb::FailReason Mount(const std::string &fileName, unsigned int priority);
    bool Mount(EmuDb* database, unsigned int priority);
    bool Unmount(EmuDb* database);

    EmuDb* GetMasterDatabase();
    std::vector<EmuDb*> GetMountedDatabases();
    void SetMountOrder(const std::vector<EmuDb*>& order);
    EmuDb* GetDbFromFilePath(const std::filesystem::path &path);
    EmuDb* GetDbFromFileName(const std::string &name);
    EmuDb* GetFirstDbWhereItemExists(ItemType type, int64_t id);

    // hidden in-memory database that is lowest priority in asset lookups
    // used for adding temporary federated catalog items to the database
    // completely invisible in things like QtGui/Dialog/DatabaseDialog.cpp
    EmuDb* GetTemporaryDatabase();

    // clears the temporary database.
    // Because the temporary database is used when players from federated master servers
    // join & their avatar items are materialized into the temp database, it can grow
    // massively when a lot of players join.
    // So this is called when servers are sweeped so that memory can be freed.
    void ClearTemporaryDatabase();

    // Copies an asset's binary into the temporary database under a fresh, collision-free SYNTHETIC id, so a
    // transient/federated asset (pulled from another master) can be served without clashing with real ids.
    // Idempotent per originKey (the same source item, e.g. "domain#id", always maps to the same synthetic
    // id). Returns the synthetic id, or 0 on failure. Reachable via the normal asset lookups as the
    // lowest-priority fallback until ClearTemporaryDatabase() drops it.
    int64_t MaterializeAsset(const std::string &originKey, int assetType, const std::string &name,
                             const std::vector<unsigned char> &assetData);
    // The synthetic id a source item was materialized to, if it has been (so a caller can rewrite an
    // avatar-fetch / asset reference to point at the materialized copy).
    std::optional<int64_t> GetMaterializedId(const std::string &originKey) const;

    // Caches an asset's binary in the temporary database under its ORIGINAL id (no remap): the on-demand
    // asset-proxy cache. Keeps the id so the engine's next fetch of it resolves locally via the scratch-db
    // fallback. No-op if the id is already cached. Returns true on success.
    bool CacheAssetInTemporary(int64_t id, const std::vector<unsigned char> &data);

    SqlDb::Response RetrieveAssetData(int64_t id, int version, std::vector<unsigned char> *dataOutput, std::string *hashOutput = nullptr);

    // Universe/place lookups across every mounted database, honoring mount priority (the first
    // database with a match wins). Each returns std::nullopt when no mounted database knows about it.
    std::optional<int64_t> GetUniverseIdForPlace(int64_t placeId);
    std::optional<int64_t> GetStartPlaceIdForUniverse(int64_t universeId);
    std::optional<int> GetUniverseAvatarType(int64_t universeId);
    std::optional<bool> GetUniverseVoiceChatEnabled(int64_t universeId);
    std::optional<std::string> GetItemName(ItemType type, int64_t id);
    std::optional<int64_t> GetCreatorUserId(ItemType type, int64_t id);

    // Toolbox/marketplace asset queries spanning every mounted database. SearchAssetIds merges the
    // per-database matches (mount priority order, de-duplicated) before applying offset/limit;
    // GetAssetSummary returns the first mounted database's copy of the asset.
    std::vector<int64_t> SearchAssetIds(Roblox::AssetType type, const std::string &keyword, int limit, int offset);
    std::optional<EmuDb::AssetSummary> GetAssetSummary(int64_t id);
    std::optional<EmuDb::AssetProductInfo> GetAssetProductInfo(int64_t id);

    // Universe enumeration spanning every mounted database, for the develop "search/universes"
    // homepage list. ListUniverseIds merges the per-database matches (mount priority order,
    // de-duplicated) before applying offset/limit; groupOwned selects user-owned (creator:User) vs
    // group-owned (creator:Team) universes. GetUniverseSummary returns the first mounted database's copy.
    std::vector<int64_t> ListUniverseIds(bool groupOwned, int limit, int offset);
    std::optional<EmuDb::UniverseSummary> GetUniverseSummary(int64_t id);

    // Every place belonging to a universe, its start place first, merged across all mounted
    // databases (mount priority order, de-duplicated). An overlay database can add places to a
    // universe whose row lives in a lower-priority mount, so this cannot stop at the first match.
    std::vector<int64_t> ListUniversePlaceIds(int64_t universeId);

    // Image/thumbnail bytes for an item, from the first mounted database that has it (falling back to
    // the highest-priority database, which yields a placeholder icon on a miss). Empty only when no
    // database is mounted.
    std::vector<unsigned char> RetrieveImageData(ItemType type, int64_t id);

    // First available place-carousel image in mount-priority order. This deliberately does not use
    // the place asset's ImageId, which is the separate square game icon.
    std::vector<unsigned char> RetrievePlaceThumbnailData(int64_t placeId);
private:
    Core* mCore;
    std::vector<EmuDb*> mMountedDatabases;
    EmuDb* mTemporaryDatabase { nullptr };           // hidden lowest-priority scratch db (see GetTemporaryDatabase)
    std::map<std::string, int64_t> mMaterializedIds; // originKey -> synthetic id in the scratch db
    int64_t mNextSynthId { 1LL << 48 };              // synthetic-id allocator, well above any real Roblox asset id
};
}
