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
// File: Quaternion.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Ported from RobloxFiles.DataTypes.Quaternion (MaximumADHD/Roblox-File-Format).
#pragma once

namespace NoobWarrior::Roblox::DataTypes {
struct Quaternion {
    float X {};
    float Y {};
    float Z {};
    float W {1.0f};

    constexpr Quaternion() = default;
    constexpr Quaternion(float x, float y, float z, float w) : X(x), Y(y), Z(z), W(w) {}

    friend constexpr bool operator==(const Quaternion &, const Quaternion &) = default;
};
}
