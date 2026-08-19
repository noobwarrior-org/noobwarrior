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
// File: ColorSequenceKeypoint.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Ported from RobloxFiles.DataTypes.ColorSequenceKeypoint (MaximumADHD/Roblox-File-Format).
#pragma once

#include <NoobWarrior/Roblox/FileFormat/DataTypes/Color3.h>

#include <cstdint>

namespace NoobWarrior::Roblox::DataTypes {
struct ColorSequenceKeypoint {
    float Time {};
    Color3 Value;
    // Envelope is an int32 on the wire and in the reference (ColorSequenceKeypoint.cs:7), unlike
    // NumberSequenceKeypoint where it is a float.
    int32_t Envelope {};

    constexpr ColorSequenceKeypoint() = default;
    constexpr ColorSequenceKeypoint(float time, Color3 value, int32_t envelope = 0) :
        Time(time), Value(value), Envelope(envelope) {}

    friend constexpr bool operator==(const ColorSequenceKeypoint &,
                                     const ColorSequenceKeypoint &) = default;
};
}
