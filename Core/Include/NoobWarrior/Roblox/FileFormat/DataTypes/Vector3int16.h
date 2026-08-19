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
// File: Vector3int16.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Ported from RobloxFiles.DataTypes.Vector3int16 (MaximumADHD/Roblox-File-Format).
#pragma once

#include <cstdint>

namespace NoobWarrior::Roblox::DataTypes {
struct Vector3int16 {
    int16_t X {};
    int16_t Y {};
    int16_t Z {};

    constexpr Vector3int16() = default;
    constexpr Vector3int16(int16_t x, int16_t y, int16_t z) : X(x), Y(y), Z(z) {}

    friend constexpr bool operator==(const Vector3int16 &, const Vector3int16 &) = default;
};
}
