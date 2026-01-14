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
// File: RobloxFile.cpp
// Started by: Hattozo
// Started on: 3/9/2025
// Description: Represents a loaded Roblox place/model file. RobloxFile is an Instance and its children are the contents of the file.
// This file is derived from Roblox-File-Format (https://github.com/MaximumADHD/Roblox-File-Format/blob/main/RobloxFile.cs)
#include <NoobWarrior/Roblox/FileFormat/RobloxFile.h>
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryRobloxFile.h>
#include <NoobWarrior/Roblox/FileFormat/XmlFormat/XmlRobloxFile.h>

using namespace NoobWarrior::Roblox;

FileResponse RobloxFile::Open(RobloxFile **file, std::vector<unsigned char> buffer) {
    *file = nullptr;
    if (buffer.size() > 14) {
        std::string header;
        header.assign(buffer.begin(), buffer.begin() + 8);

        if (header.compare("<roblox!") == 0)
            *file = new BinaryRobloxFile();
        else if (header.starts_with("<roblox"))
            *file = new XmlRobloxFile();

        if (*file != nullptr) {
            return (*file)->ReadFile(buffer);
        }
    }
    return FileResponse::InvalidHeader;
}

FileResponse RobloxFile::Open(RobloxFile **file, std::string_view filePath) {
    return FileResponse::Failed;
}