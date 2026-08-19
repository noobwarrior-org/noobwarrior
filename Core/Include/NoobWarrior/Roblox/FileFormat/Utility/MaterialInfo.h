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
// File: MaterialInfo.h
// Description: Ported from RobloxFiles.MaterialInfo; flags for the PhysicalProperties column.
#pragma once

#include <cstdint>

namespace NoobWarrior::Roblox {
// The leading byte of each PhysicalProperties value. CustomPhysics gates the five float fields;
// AcousticAbsorption adds a sixth. A byte can carry AcousticAbsorption alone, in which case no
// floats follow at all.
enum class MaterialBitFlags : uint8_t {
    None = 0x0,
    CustomPhysics = 0x1,
    AcousticAbsorption = 0x2,
};

constexpr bool HasFlag(uint8_t flags, MaterialBitFlags flag) {
    return (flags & static_cast<uint8_t>(flag)) != 0;
}

constexpr uint8_t WithFlag(uint8_t flags, MaterialBitFlags flag) {
    return static_cast<uint8_t>(flags | static_cast<uint8_t>(flag));
}
}
