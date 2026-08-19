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
// File: SSTR.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Shared string table chunk.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryFileReader.h>
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryFileWriter.h>

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace NoobWarrior::Roblox {
// Shared strings are addressed by their position in this table: a SharedString property column
// stores indices into Strings.
//
// RobloxFiles keys its lookup dictionary on a base64 rendering of the 16-byte hash. Here the raw
// bytes are kept instead, since std::string/array are byte-safe and that avoids pulling a base64
// codec into the format layer. The serialized bytes are identical either way.
class SSTR {
public:
    static constexpr int32_t kFormat = 0;

    struct Entry {
        std::array<unsigned char, 16> Hash {};
        std::vector<unsigned char> Value;

        friend bool operator==(const Entry &, const Entry &) = default;
    };

    // Ordered, because the index is the id referenced by property columns and because preserving
    // order is what makes a re-encode byte-identical.
    std::vector<Entry> Strings;

    std::optional<uint32_t> Find(const std::array<unsigned char, 16> &hash) const;
    uint32_t Add(const Entry &entry);

    bool Load(BinaryRobloxFileReader &reader);
    void Save(BinaryRobloxFileWriter &writer) const;
};
}
