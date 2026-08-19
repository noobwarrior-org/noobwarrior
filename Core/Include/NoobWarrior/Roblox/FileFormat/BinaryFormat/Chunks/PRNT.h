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
// File: PRNT.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Parent-child association chunk.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryFileReader.h>
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryFileWriter.h>

#include <cstdint>
#include <vector>

namespace NoobWarrior::Roblox {
class PRNT {
public:
    static constexpr uint8_t kFormat = 0;

    std::vector<int32_t> ChildIds;
    std::vector<int32_t> ParentIds;

    bool Load(BinaryRobloxFileReader &reader);
    void Save(BinaryRobloxFileWriter &writer) const;
};
}
