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
// File: SharedString.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Ported from RobloxFiles.DataTypes.SharedString (MaximumADHD/Roblox-File-Format).
#pragma once

#include <string>
#include <vector>

namespace NoobWarrior::Roblox::DataTypes {
// Key is the 16-byte MD5 digest the SSTR chunk indexes entries by, held as raw bytes.
struct SharedString {
    std::string Key;
    std::vector<unsigned char> Value;

    SharedString() = default;
    SharedString(std::string key, std::vector<unsigned char> value) :
        Key(std::move(key)), Value(std::move(value)) {}

    friend bool operator==(const SharedString &, const SharedString &) = default;
};
}
