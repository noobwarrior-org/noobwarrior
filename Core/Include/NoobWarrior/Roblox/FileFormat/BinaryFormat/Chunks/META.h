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
// File: META.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: File metadata chunk.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryFileReader.h>
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryFileWriter.h>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace NoobWarrior::Roblox {
class META {
public:
    // A vector rather than a map: entry order is part of the serialized form, so preserving it is
    // what lets a re-encode match the source file byte for byte.
    std::vector<std::pair<std::string, std::string>> Data;

    const std::string *Find(std::string_view key) const;

    bool Load(BinaryRobloxFileReader &reader);
    void Save(BinaryRobloxFileWriter &writer) const;
};
}
