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
// File: INST.cpp
// Started by: Hattozo
// Started on: 8/18/2025
// Description: Class descriptor chunk: one per distinct ClassName in the file.

#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/Chunks/INST.h>

using namespace NoobWarrior::Roblox;

bool INST::Load(BinaryRobloxFileReader &reader) {
    ClassIndex = reader.ReadInt32();
    ClassName = reader.ReadString();
    IsService = reader.ReadByte() != 0;
    NumObjects = reader.ReadInt32();
    if (reader.Failed() || NumObjects < 0)
        return false;

    ObjectIds = reader.ReadReferents(static_cast<size_t>(NumObjects));
    if (reader.Failed())
        return false;

    RootedServices.clear();
    if (IsService) {
        RootedServices.reserve(static_cast<size_t>(NumObjects));
        for (int32_t index = 0; index < NumObjects; ++index)
            RootedServices.push_back(reader.ReadByte() != 0);
    }
    return !reader.Failed();
}

void INST::Save(BinaryRobloxFileWriter &writer) const {
    writer.WriteInt32(ClassIndex);
    writer.WriteString(ClassName);
    writer.WriteByte(IsService ? 1 : 0);
    writer.WriteInt32(NumObjects);
    writer.WriteReferents(ObjectIds);

    if (!IsService)
        return;
    for (size_t index = 0; index < static_cast<size_t>(NumObjects); ++index) {
        const bool rooted = index < RootedServices.size() && RootedServices[index];
        writer.WriteByte(rooted ? 1 : 0);
    }
}
