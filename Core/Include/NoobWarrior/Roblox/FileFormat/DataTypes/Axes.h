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
// File: Axes.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Ported from RobloxFiles.DataTypes.Axes (MaximumADHD/Roblox-File-Format).
#pragma once

#include <cstdint>

namespace NoobWarrior::Roblox::DataTypes {
// Serialized as a single byte whose low three bits follow Enum.Axis ordering.
struct Axes {
    bool X {};
    bool Y {};
    bool Z {};

    constexpr Axes() = default;
    explicit constexpr Axes(uint8_t flags) : X(flags & 1), Y(flags & 2), Z(flags & 4) {}

    constexpr uint8_t Flags() const {
        return static_cast<uint8_t>((X ? 1 : 0) | (Y ? 2 : 0) | (Z ? 4 : 0));
    }

    friend constexpr bool operator==(const Axes &, const Axes &) = default;
};
}
