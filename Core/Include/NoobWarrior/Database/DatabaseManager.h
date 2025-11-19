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

    std::vector<unsigned char> RetrieveAssetData(int64_t id);

    AssetRepositoryManager& GetAssetRepository();
private:
    std::vector<Database*> MountedDatabases;
    AssetRepositoryManager mAssetRepository;
};

template<typename Item, typename RepositoryClass>
class ItemRepositoryManager : public ItemRepository<Item> {
public:
    ItemRepositoryManager(DatabaseManager *dbMgr) : mDbMgr(dbMgr) {}
    virtual RepositoryClass& GetRepository(Database *db) = 0;
    DatabaseResponse SaveItem(const Item &item) override {
        return GetRepository(mDbMgr->GetMasterDatabase())->SaveItem(item);
    }
    DatabaseResponse RemoveItem(int64_t id) override {
        return GetRepository(mDbMgr->GetMasterDatabase())->RemoveItem(id);
    }
    DatabaseResponse RemoveItemSnapshot(int64_t id, int snapshot) override {
        return GetRepository(mDbMgr->GetMasterDatabase())->RemoveItemSnapshot(id, snapshot);
    }
    DatabaseResponse MoveItem(int64_t currentId, int64_t newId) override {
        return GetRepository(mDbMgr->GetMasterDatabase())->MoveItem(currentId, newId);
    }
    std::optional<Item> GetItemById(int64_t id) override {
        for (Database *db : mDbMgr->GetMountedDatabases()) {
            std::optional<Item> item = GetRepository(db)->GetItemById(id);
            if (item.has_value())
                return item.value();
        }
    }
    std::optional<Item> GetItemById(int64_t id, int snapshot) override {
        for (Database *db : mDbMgr->GetMountedDatabases()) {
            std::optional<Item> item = GetRepository(db)->GetItemById(id, snapshot);
            if (item.has_value())
                return item.value();
        }
    }
    std::vector<Item> ListItems() override {
        std::vector<Item> allItems;
        for (Database *db : mDbMgr->GetMountedDatabases()) {
            std::vector<Item> items = GetRepository(db)->ListItems();
            allItems.insert(allItems.end(), items.begin(), items.end());
        }
        return allItems;
    }
    bool DoesItemExist(int64_t id) override {
        for (int i = 0; i < mDbMgr->GetMountedDatabases().size(); i++) {
            Database *database = mDbMgr->GetMountedDatabases()[i];
            if (const bool exists = GetRepository(database)->DoesItemExist())
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