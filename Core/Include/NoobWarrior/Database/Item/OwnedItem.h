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
// File: OwnedItem.h
// Started by: Hattozo
// Started on: N/A
// Description:
#pragma once
#include "Item.h"
#include <NoobWarrior/Roblox/Api/Asset.h>

#include <optional>

namespace NoobWarrior {
struct OwnedItem : Item {
    Roblox::CreatorType CreatorType;
    int64_t             CreatorId;
    std::string         Description;
    int64_t             Created;
    int64_t             Updated;
    int64_t             ImageId;
    int                 ImageSnapshot;
};
}
