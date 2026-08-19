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
// File: Color3.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Ported from RobloxFiles.DataTypes.Color3 (MaximumADHD/Roblox-File-Format).
#pragma once

#include <cstdint>

namespace NoobWarrior::Roblox::DataTypes {
// The file-format Color3 is float-based, matching the serialized representation. It is distinct
// from NoobWarrior::Roblox::Color3, which the emulator's avatar code uses with double precision.
struct Color3 {
    float R {};
    float G {};
    float B {};

    constexpr Color3() = default;
    constexpr Color3(float r, float g, float b) : R(r), G(g), B(b) {}

    static constexpr Color3 FromRGB(uint8_t r, uint8_t g, uint8_t b) {
        return {r / 255.0f, g / 255.0f, b / 255.0f};
    }

    friend constexpr bool operator==(const Color3 &, const Color3 &) = default;
};
}
