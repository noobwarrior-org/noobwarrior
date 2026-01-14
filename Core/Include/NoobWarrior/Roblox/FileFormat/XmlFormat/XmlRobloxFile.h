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
// File: XmlRobloxFile.h
// Started by: Hattozo
// Started on: 3/9/2025
// Description:
#pragma once
#include <NoobWarrior/Roblox/FileFormat/RobloxFile.h>
#include <NoobWarrior/Roblox/FileFormat/Tree/Instance.h>

#include <pugixml.hpp>

#include <map>
#include <vector>
#include <unordered_set>

namespace NoobWarrior::Roblox {
class XmlRobloxFile : public RobloxFile {
public:
    XmlRobloxFile();

    pugi::xml_document XmlDocument;

    std::map<std::string, Instance> Instances;
    std::map<std::string, std::string> Metadata;
    std::unordered_set<std::string> SharedStrings;

    void Save() override;
protected:
    FileResponse ReadFile(std::vector<unsigned char> buffer) override;
private:
    int RefCounter;
};
}