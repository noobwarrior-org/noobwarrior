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
// File: Region3int16.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Ported from RobloxFiles.DataTypes.Region3int16 (MaximumADHD/Roblox-File-Format).
#pragma once

#include <NoobWarrior/Roblox/FileFormat/DataTypes/Vector3int16.h>

namespace NoobWarrior::Roblox::DataTypes {
struct Region3int16 {
    Vector3int16 Min;
    Vector3int16 Max;

    constexpr Region3int16() = default;
    constexpr Region3int16(Vector3int16 min, Vector3int16 max) : Min(min), Max(max) {}

    friend constexpr bool operator==(const Region3int16 &, const Region3int16 &) = default;
};
}
