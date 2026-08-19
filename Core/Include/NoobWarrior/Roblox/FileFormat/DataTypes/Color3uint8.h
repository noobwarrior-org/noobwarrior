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
// File: Color3uint8.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Ported from RobloxFiles.DataTypes.Color3uint8 (MaximumADHD/Roblox-File-Format).
#pragma once

#include <NoobWarrior/Roblox/FileFormat/DataTypes/Color3.h>

#include <cstdint>

namespace NoobWarrior::Roblox::DataTypes {
struct Color3uint8 {
    uint8_t R {};
    uint8_t G {};
    uint8_t B {};

    constexpr Color3uint8() = default;
    constexpr Color3uint8(uint8_t r, uint8_t g, uint8_t b) : R(r), G(g), B(b) {}

    constexpr Color3 ToColor3() const { return Color3::FromRGB(R, G, B); }

    friend constexpr bool operator==(const Color3uint8 &, const Color3uint8 &) = default;
};
}
