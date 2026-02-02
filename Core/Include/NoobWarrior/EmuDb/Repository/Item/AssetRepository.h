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
// File: AssetRepository.h
// Started by: Hattozo
// Started on: 11/11/2025
// Description:
#pragma once
#include <NoobWarrior/EmuDb/Repository/Repository.h>
#include <NoobWarrior/EmuDb/Item/Asset.h>

namespace NoobWarrior {
class AssetRepository : public ItemRepository<Asset> {
public:
    AssetRepository(EmuDb *db);
    std::vector<unsigned char> RetrieveData(int64_t id, int snapshot);
    std::vector<unsigned char> RetrieveData(int64_t id);
    
    SqlDb::Response Save(const Asset &asset) override;
    SqlDb::Response Remove(int64_t id, int snapshot) override;
    SqlDb::Response Remove(int64_t id) override;
    SqlDb::Response Move(int64_t currentId, int64_t newId) override;
    std::optional<Asset> Get(int64_t id) override;
    std::optional<Asset> Get(int64_t id, int snapshot) override;
    std::vector<Asset> List() override;

    bool Exists(int64_t id, int snapshot) override;
    bool Exists(int64_t id) override;
};
}