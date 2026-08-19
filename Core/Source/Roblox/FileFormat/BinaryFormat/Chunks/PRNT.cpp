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
// File: PRNT.cpp
// Started by: Hattozo
// Started on: 8/18/2025
// Description: Parent-child association chunk.

#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/Chunks/PRNT.h>

using namespace NoobWarrior::Roblox;

bool PRNT::Load(BinaryRobloxFileReader &reader) {
    const uint8_t format = reader.ReadByte();
    const int32_t idCount = reader.ReadInt32();
    if (reader.Failed() || format != kFormat || idCount < 0)
        return false;

    ChildIds = reader.ReadReferents(static_cast<size_t>(idCount));
    ParentIds = reader.ReadReferents(static_cast<size_t>(idCount));
    return !reader.Failed() && ChildIds.size() == ParentIds.size();
}

void PRNT::Save(BinaryRobloxFileWriter &writer) const {
    writer.WriteByte(kFormat);
    writer.WriteInt32(static_cast<int32_t>(ChildIds.size()));
    writer.WriteReferents(ChildIds);
    writer.WriteReferents(ParentIds);
}
