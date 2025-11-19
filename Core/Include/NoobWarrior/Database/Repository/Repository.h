// === noobWarrior ===
// File: Repository.h
// Started by: Hattozo
// Started on: 11/11/2025
// Description: The infamous "repository pattern" seen in database abstractions. Let's see how well it works.
// Apparently this is very common in ASP.Net servers.
// I imagine Roblox themselves used this pattern when designing their backend.
#pragma once
#include <NoobWarrior/Database/Common.h>

#include <vector>
#include <optional>
#include <cstdint>

namespace NoobWarrior {
class Database;

template<typename Item>
class Repository {
public:
    Repository(Database *db) : mDb(db) {}
    virtual DatabaseResponse SaveItem(const Item &item) = 0;
    virtual DatabaseResponse RemoveItem(int64_t id) = 0;
    virtual DatabaseResponse MoveItem(int64_t currentId, int64_t newId) = 0;
    virtual std::optional<Item> GetItemById(int64_t id) = 0;
    virtual std::vector<Item> ListItems() = 0;
    virtual bool DoesItemExist(int64_t id) = 0;
protected:
    Database *mDb;
};

template<typename Item>
class ItemRepository : public Repository<Item> {
public:
    ItemRepository(Database *db) : Repository<Item>(db) {}
    virtual DatabaseResponse RemoveItemSnapshot(int64_t id, int snapshot) = 0;
    virtual std::optional<Item> GetItemById(int64_t id, int snapshot) = 0;
};
}