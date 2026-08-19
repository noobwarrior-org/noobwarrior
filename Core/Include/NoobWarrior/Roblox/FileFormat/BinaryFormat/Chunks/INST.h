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
// File: INST.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Class descriptor chunk: one per distinct ClassName in the file.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryFileReader.h>
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryFileWriter.h>

#include <cstdint>
#include <string>
#include <vector>

namespace NoobWarrior::Roblox {
// Roblox emits exactly one INST chunk per distinct class name, so ClassName is effectively the
// key of the file's class table.
class INST {
public:
    int32_t ClassIndex {};
    std::string ClassName;
    bool IsService {};
    std::vector<bool> RootedServices;
    int32_t NumObjects {};
    std::vector<int32_t> ObjectIds;

    // Reads the chunk body. The object graph is populated separately, once BinaryRobloxFile can
    // construct instances for each referent.
    bool Load(BinaryRobloxFileReader &reader);
    void Save(BinaryRobloxFileWriter &writer) const;
};
}
