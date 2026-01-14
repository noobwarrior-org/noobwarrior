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
// File: RobloxFile.h
// Started by: Hattozo
// Started on: 3/9/2025
// Description:
#pragma once

#include "Tree/Instance.h"

#include <string_view>
#include <vector>

namespace NoobWarrior::Roblox {
enum class FileResponse {
    Failed,
    Success,
    InvalidHeader,
    CouldNotParse,
    InvalidVersion,
    VersionTooLow
};

class RobloxFile : public Instance {
public:
    static bool LogErrors;

    // RobloxFile(char *buffer);
    // RobloxFile(const std::filesystem::path &filePath);

    static FileResponse Open(RobloxFile **file, std::vector<unsigned char> buffer);
    static FileResponse Open(RobloxFile **file, std::string_view filePath);

    virtual void Save() = 0;
protected:
    virtual FileResponse ReadFile(std::vector<unsigned char> buffer) = 0;
};
}