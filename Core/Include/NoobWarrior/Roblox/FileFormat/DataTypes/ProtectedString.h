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
// File: ProtectedString.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Ported from RobloxFiles.DataTypes.ProtectedString (MaximumADHD/Roblox-File-Format).
#pragma once

#include <string>
#include <vector>

namespace NoobWarrior::Roblox::DataTypes {
// Serialized exactly like String: an int32 length followed by raw bytes. Script.Source uses it.
struct ProtectedString {
    std::vector<unsigned char> RawBuffer;

    ProtectedString() = default;
    explicit ProtectedString(std::vector<unsigned char> buffer) : RawBuffer(std::move(buffer)) {}
    explicit ProtectedString(const std::string &value) :
        RawBuffer(value.begin(), value.end()) {}

    std::string ToString() const { return {RawBuffer.begin(), RawBuffer.end()}; }

    friend bool operator==(const ProtectedString &, const ProtectedString &) = default;
};
}
