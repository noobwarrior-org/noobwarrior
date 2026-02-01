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
// File: Item.h
// Started by: Hattozo
// Started on: 11/11/2025
// Description: Base class for all items (assets, groups, games, users)
#pragma once
#include <cstdint>
#include <string>

#include "../ContentImages.h"

namespace NoobWarrior {
struct Item {
    int64_t             Id;
    int                 Snapshot;
    int64_t             FirstRecorded;
    int64_t             LastRecorded;
    std::string         Name;

    static constexpr std::string TypeName = "Item";
    static constexpr const int* DefaultImage = g_icon_content_deleted;
    inline static const int DefaultImageSize = g_icon_content_deleted_size;
};
}
