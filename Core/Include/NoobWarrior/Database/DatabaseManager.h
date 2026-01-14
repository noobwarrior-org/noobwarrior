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
#include <NoobWarrior/Database/Database.h>
#include <NoobWarrior/Database/Repository/Repository.h>

#include <filesystem>
#include <vector>

namespace NoobWarrior {
class DatabaseManager {
public:
    DatabaseResponse AutocreateMasterDatabase();

    DatabaseResponse Mount(const std::filesystem::path &filePath, unsigned int priority);
    void Mount(Database *database, unsigned int priority);
    int Unmount(Database *database);

    Database *GetMasterDatabase();
    std::vector<Database*> GetMountedDatabases();

    bool GetUserFromToken(User *user, const std::string &token);

    // TODO: PLEASE FIX THE CIRCULATORY DEPENDENCIES. FUCK.
    // AssetRepositoryManager& GetAssetRepository();
private:
    std::vector<Database*> MountedDatabases;
    // AssetRepositoryManager mAssetRepository;
};

template<typename Item, typename RepositoryClass>
class ItemRepositoryManager : public ItemRepository<Item> {
public:
    ItemRepositoryManager(DatabaseManager *dbMgr) : mDbMgr(dbMgr) {}
    virtual RepositoryClass& GetRepository(Database *db) = 0;
    DatabaseResponse Save(const Item &item) override {
        return GetRepository(mDbMgr->GetMasterDatabase())->Save(item);
    }
    DatabaseResponse Remove(int64_t id) override {
        return GetRepository(mDbMgr->GetMasterDatabase())->Remove(id);
    }
    DatabaseResponse Remove(int64_t id, int snapshot) override {
        return GetRepository(mDbMgr->GetMasterDatabase())->Remove(id, snapshot);
    }
    DatabaseResponse Move(int64_t currentId, int64_t newId) override {
        return GetRepository(mDbMgr->GetMasterDatabase())->Move(currentId, newId);
    }
    std::optional<Item> Get(int64_t id) override {
        for (Database *db : mDbMgr->GetMountedDatabases()) {
            std::optional<Item> item = GetRepository(db)->Get(id);
            if (item.has_value())
                return item.value();
        }
    }
    std::optional<Item> Get(int64_t id, int snapshot) override {
        for (Database *db : mDbMgr->GetMountedDatabases()) {
            std::optional<Item> item = GetRepository(db)->Get(id, snapshot);
            if (item.has_value())
                return item.value();
        }
    }
    std::vector<Item> List() override {
        std::vector<Item> allItems;
        for (Database *db : mDbMgr->GetMountedDatabases()) {
            std::vector<Item> items = GetRepository(db)->List();
            allItems.insert(allItems.end(), items.begin(), items.end());
        }
        return allItems;
    }
    bool Exists(int64_t id) override {
        for (int i = 0; i < mDbMgr->GetMountedDatabases().size(); i++) {
            Database *database = mDbMgr->GetMountedDatabases()[i];
            if (const bool exists = GetRepository(database)->Exists())
                return exists;
        }
        return false;
    }
protected:
    DatabaseManager *mDbMgr;
};

class AssetRepositoryManager : public ItemRepositoryManager<Asset, AssetRepository> {
public:
    inline AssetRepository& GetRepository(Database *db) override {
        return db->GetAssetRepository();
    }
};
}