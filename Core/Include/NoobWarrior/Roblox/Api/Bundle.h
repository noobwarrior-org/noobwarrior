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
// File: Bundle.h (Roblox API)
// Started by: Hattozo
// Started on: 9/20/2025
// Description: A C-struct representation of what the Roblox API would give you when you ask it for a bundle.
#pragma once

namespace NoobWarrior::Roblox {
    constexpr int BundleTypeCount = 5;
    // Note: "None" is a fallback if the asset does not have a type. It's not a real value in the Roblox API
    enum class BundleType { None = 0, BodyParts = 1, Animations = 2, Shoes = 3, DynamicHead = 4, DynamicHeadAvatar = 5 };
}