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
// File: PROP.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Decodes and re-encodes one PROP chunk value column.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryFileReader.h>
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryFileWriter.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/DataTypes.h>
#include <NoobWarrior/Roblox/FileFormat/Tree/Property.h>

#include <any>
#include <cstdint>
#include <string>
#include <vector>

namespace NoobWarrior::Roblox {
class PROP {
public:
    // One PROP chunk's value column, decoded. Values holds one std::any per instance in the class,
    // typed per PROP::ValueTypeFor.
    //
    // Two members exist purely so a decode/encode cycle is byte-identical to the source file, which
    // the reference implementation does not guarantee:
    //   * OrientIds preserves whether each CFrame was stored as a packed orientation id or a raw 3x3
    //     matrix. Recomputing it would rewrite axis-aligned matrices that a file chose to store raw.
    //   * ExternalContentObjects preserves Content's trailing external-object referent list, which
    //     RobloxFiles reads and then discards.
    struct ValueColumn {
        PropertyType Type {PropertyType::Unknown};
        std::vector<std::any> Values;
        std::vector<uint8_t> OrientIds;
        // Per-value PhysicalProperties flag byte. A byte of 0x02 (acoustic absorption without custom
        // physics) carries no float payload, so the decoded value alone cannot reproduce it.
        std::vector<uint8_t> RawFlags;
        std::vector<int32_t> ExternalContentObjects;
    };

    uint32_t ClassIndex {};
    std::string Name;
    PropertyType Type {PropertyType::Unknown};

    // Decodes count values of the given type. Returns false if the column is malformed or the
    // type is not serializable, leaving the reader's failure flag set.
    static bool Read(BinaryRobloxFileReader &reader, PropertyType type, size_t count,
                     PROP::ValueColumn &column);

    // Re-encodes a column produced by Read. Returns false if a value holds an unexpected type.
    static bool Write(BinaryRobloxFileWriter &writer, const PROP::ValueColumn &column);

    // True when this codec can round-trip the type at all.
    static bool IsSupported(PropertyType type);
};
}
