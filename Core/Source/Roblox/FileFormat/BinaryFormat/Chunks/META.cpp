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
// File: META.cpp
// Started by: Hattozo
// Started on: 8/18/2025
// Description: File metadata chunk.

#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/Chunks/META.h>

using namespace NoobWarrior::Roblox;

const std::string *META::Find(std::string_view key) const {
    for (const auto &[name, value] : Data) {
        if (name == key)
            return &value;
    }
    return nullptr;
}

bool META::Load(BinaryRobloxFileReader &reader) {
    const int32_t count = reader.ReadInt32();
    if (reader.Failed() || count < 0)
        return false;

    Data.clear();
    Data.reserve(static_cast<size_t>(count));
    for (int32_t index = 0; index < count; ++index) {
        std::string key = reader.ReadString();
        std::string value = reader.ReadString();
        if (reader.Failed())
            return false;
        Data.emplace_back(std::move(key), std::move(value));
    }
    return true;
}

void META::Save(BinaryRobloxFileWriter &writer) const {
    writer.WriteInt32(static_cast<int32_t>(Data.size()));
    for (const auto &[key, value] : Data) {
        writer.WriteString(key);
        writer.WriteString(value);
    }
}
