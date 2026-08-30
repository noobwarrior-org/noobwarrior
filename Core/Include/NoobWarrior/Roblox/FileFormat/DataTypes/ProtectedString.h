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

    // Set when the buffer came in as bytes that look like Luau byte-code. ProtectedString.cs:29-38
    // decides that on the first byte: a VM version is below 32, so anything at or above it is text
    // that merely arrived as bytes. A compiled source cannot be written as XML text.
    bool IsCompiled {};

    ProtectedString() = default;
    explicit ProtectedString(std::vector<unsigned char> buffer) :
        RawBuffer(std::move(buffer)),
        IsCompiled(!RawBuffer.empty() && RawBuffer.front() < 32) {}
    explicit ProtectedString(const std::string &value) :
        RawBuffer(value.begin(), value.end()) {}

    // The reference's ToString() reports "byte[N]" for a compiled buffer because it is a display
    // override. This one is what PROP's String column is written from, so it always hands back the
    // bytes; use IsCompiled to decide how to present them.
    std::string ToString() const { return {RawBuffer.begin(), RawBuffer.end()}; }

    friend bool operator==(const ProtectedString &, const ProtectedString &) = default;
};
}
