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
// File: Region3.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Ported from RobloxFiles.DataTypes.Region3 (MaximumADHD/Roblox-File-Format).
#pragma once

#include <NoobWarrior/Roblox/FileFormat/DataTypes/CFrame.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/Vector3.h>

namespace NoobWarrior::Roblox::DataTypes {
struct Region3 {
    Vector3 Min;
    Vector3 Max;

    constexpr Region3() = default;
    constexpr Region3(Vector3 min, Vector3 max) : Min(min), Max(max) {}

    constexpr Vector3 Size() const { return Max - Min; }
    constexpr CFrame GetCFrame() const { return CFrame((Min + Max) * 0.5f); }

    friend constexpr bool operator==(const Region3 &, const Region3 &) = default;
};
}
