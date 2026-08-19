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
// File: Faces.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Ported from RobloxFiles.DataTypes.Faces (MaximumADHD/Roblox-File-Format).
#pragma once

#include <cstdint>

namespace NoobWarrior::Roblox::DataTypes {
// Serialized as a single byte whose bits follow Enum.NormalId ordering.
struct Faces {
    bool Right {};
    bool Top {};
    bool Back {};
    bool Left {};
    bool Bottom {};
    bool Front {};

    constexpr Faces() = default;
    explicit constexpr Faces(uint8_t flags) :
        Right(flags & 1), Top(flags & 2), Back(flags & 4),
        Left(flags & 8), Bottom(flags & 16), Front(flags & 32) {}

    constexpr uint8_t Flags() const {
        return static_cast<uint8_t>((Right ? 1 : 0) | (Top ? 2 : 0) | (Back ? 4 : 0) |
                                    (Left ? 8 : 0) | (Bottom ? 16 : 0) | (Front ? 32 : 0));
    }

    friend constexpr bool operator==(const Faces &, const Faces &) = default;
};
}
