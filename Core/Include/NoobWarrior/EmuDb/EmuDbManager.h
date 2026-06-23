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

    SqlDb::Response RetrieveAssetData(int64_t id, int version, std::vector<unsigned char> *dataOutput, std::string *hashOutput = nullptr);

    // Universe/place lookups across every mounted database, honoring mount priority (the first
    // database with a match wins). Each returns std::nullopt when no mounted database knows about it.
    std::optional<int64_t> GetUniverseIdForPlace(int64_t placeId);
    std::optional<int64_t> GetStartPlaceIdForUniverse(int64_t universeId);
    std::optional<std::string> GetItemName(ItemType type, int64_t id);
    std::optional<int64_t> GetCreatorUserId(ItemType type, int64_t id);

    // Toolbox/marketplace asset queries spanning every mounted database. SearchAssetIds merges the
    // per-database matches (mount priority order, de-duplicated) before applying offset/limit;
    // GetAssetSummary returns the first mounted database's copy of the asset.
    std::vector<int64_t> SearchAssetIds(Roblox::AssetType type, const std::string &keyword, int limit, int offset);
    std::optional<EmuDb::AssetSummary> GetAssetSummary(int64_t id);

    // Universe enumeration spanning every mounted database, for the develop "search/universes"
    // homepage list. ListUniverseIds merges the per-database matches (mount priority order,
    // de-duplicated) before applying offset/limit; groupOwned selects user-owned (creator:User) vs
    // group-owned (creator:Team) universes. GetUniverseSummary returns the first mounted database's copy.
    std::vector<int64_t> ListUniverseIds(bool groupOwned, int limit, int offset);
    std::optional<EmuDb::UniverseSummary> GetUniverseSummary(int64_t id);

    // Image/thumbnail bytes for an item, from the first mounted database that has it (falling back to
    // the highest-priority database, which yields a placeholder icon on a miss). Empty only when no
    // database is mounted.
    std::vector<unsigned char> RetrieveImageData(ItemType type, int64_t id);
private:
    Core* mCore;
    std::vector<EmuDb*> mMountedDatabases;
};
}