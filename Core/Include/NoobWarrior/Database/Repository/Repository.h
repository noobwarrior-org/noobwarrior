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
    virtual DatabaseResponse Save(const Item &item) = 0;
    virtual DatabaseResponse Remove(int64_t id) = 0;
    virtual DatabaseResponse Move(int64_t currentId, int64_t newId) = 0;
    virtual std::optional<Item> Get(int64_t id) = 0;
    virtual std::vector<Item> List() = 0;
    virtual bool Exists(int64_t id) = 0;
protected:
    Database *mDb;
};

template<typename Item>
class ItemRepository : public Repository<Item> {
public:
    ItemRepository(Database *db) : Repository<Item>(db) {}
    virtual DatabaseResponse Remove(int64_t id, int snapshot) = 0;
    virtual std::optional<Item> Get(int64_t id, int snapshot) = 0;
    virtual bool Exists(int64_t id, int snapshot) = 0;

    std::vector<unsigned char> GetIconData();
};
}