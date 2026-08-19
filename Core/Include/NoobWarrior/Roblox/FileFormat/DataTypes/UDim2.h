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
// File: UDim2.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Ported from RobloxFiles.DataTypes.UDim2 (MaximumADHD/Roblox-File-Format).
#pragma once

#include <NoobWarrior/Roblox/FileFormat/DataTypes/UDim.h>

namespace NoobWarrior::Roblox::DataTypes {
struct UDim2 {
    UDim X;
    UDim Y;

    constexpr UDim2() = default;
    constexpr UDim2(UDim x, UDim y) : X(x), Y(y) {}
    constexpr UDim2(float xScale, int32_t xOffset, float yScale, int32_t yOffset) :
        X(xScale, xOffset), Y(yScale, yOffset) {}

    friend constexpr bool operator==(const UDim2 &, const UDim2 &) = default;
};
}
