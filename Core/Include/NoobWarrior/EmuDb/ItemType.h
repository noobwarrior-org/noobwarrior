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
// File: ItemType.h
// Started by: Hattozo
// Started on: 2/14/2026
// Description:
#pragma once
#include <string>

namespace NoobWarrior {
constexpr int ItemTypeCount = 9;
enum class ItemType {
    Asset,
    Badge,
    Bundle,
    DevProduct,
    Group,
    Outfit,
    Pass,
    Set,
    Universe,
    User
};

inline std::string ItemTypeAsTranslatableString(ItemType type) {
    switch (type) {
    case ItemType::Asset: return "Asset";
    case ItemType::Badge: return "Badge";
    case ItemType::Bundle: return "Bundle";
    case ItemType::DevProduct: return "Dev Product";
    case ItemType::Group: return "Group";
    case ItemType::Outfit: return "Outfit";
    case ItemType::Pass: return "Pass";
    case ItemType::Set: return "Set";
    case ItemType::Universe: return "Universe";
    case ItemType::User: return "User";
    default: return "Unknown";
    }
};
}