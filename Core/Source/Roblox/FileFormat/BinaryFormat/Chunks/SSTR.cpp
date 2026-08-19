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
// File: SSTR.cpp
// Started by: Hattozo
// Started on: 8/18/2025
// Description: Shared string table chunk.

#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/Chunks/SSTR.h>

using namespace NoobWarrior::Roblox;

std::optional<uint32_t> SSTR::Find(const std::array<unsigned char, 16> &hash) const {
    for (size_t index = 0; index < Strings.size(); ++index) {
        if (Strings[index].Hash == hash)
            return static_cast<uint32_t>(index);
    }
    return std::nullopt;
}

uint32_t SSTR::Add(const Entry &entry) {
    if (const std::optional<uint32_t> existing = Find(entry.Hash))
        return *existing;
    Strings.push_back(entry);
    return static_cast<uint32_t>(Strings.size() - 1);
}

bool SSTR::Load(BinaryRobloxFileReader &reader) {
    const int32_t format = reader.ReadInt32();
    const int32_t count = reader.ReadInt32();
    if (reader.Failed() || format != kFormat || count < 0)
        return false;

    Strings.clear();
    Strings.reserve(static_cast<size_t>(count));
    for (int32_t index = 0; index < count; ++index) {
        Entry entry;
        if (!reader.ReadBytes(entry.Hash.data(), entry.Hash.size()))
            return false;
        entry.Value = reader.ReadRawString();
        if (reader.Failed())
            return false;
        Strings.push_back(std::move(entry));
    }
    return true;
}

void SSTR::Save(BinaryRobloxFileWriter &writer) const {
    writer.WriteInt32(kFormat);
    writer.WriteInt32(static_cast<int32_t>(Strings.size()));
    for (const Entry &entry : Strings) {
        writer.WriteBytes(entry.Hash.data(), entry.Hash.size());
        writer.WriteRawString(entry.Value);
    }
}
