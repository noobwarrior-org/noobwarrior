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
// File: Ray.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Ported from RobloxFiles.DataTypes.Ray (MaximumADHD/Roblox-File-Format).
#pragma once

#include <NoobWarrior/Roblox/FileFormat/DataTypes/Vector3.h>

namespace NoobWarrior::Roblox::DataTypes {
struct Ray {
    Vector3 Origin;
    Vector3 Direction;

    constexpr Ray() = default;
    constexpr Ray(Vector3 origin, Vector3 direction) : Origin(origin), Direction(direction) {}

    friend constexpr bool operator==(const Ray &, const Ray &) = default;
};
}
