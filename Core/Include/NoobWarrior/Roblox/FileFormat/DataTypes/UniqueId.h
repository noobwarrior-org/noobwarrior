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
// File: UniqueId.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Ported from RobloxFiles.DataTypes.UniqueId (MaximumADHD/Roblox-File-Format).
#pragma once

#include <cstdint>
#include <format>
#include <string>

namespace NoobWarrior::Roblox::DataTypes {
struct UniqueId {
    int64_t Random {};
    uint32_t Time {};
    uint32_t Index {};

    constexpr UniqueId() = default;
    constexpr UniqueId(int64_t random, uint32_t time, uint32_t index) :
        Random(random), Time(time), Index(index) {}

    std::string ToString() const {
        return std::format("{:016x}{:08x}{:08x}", static_cast<uint64_t>(Random), Time, Index);
    }

    friend constexpr bool operator==(const UniqueId &, const UniqueId &) = default;
};
}
