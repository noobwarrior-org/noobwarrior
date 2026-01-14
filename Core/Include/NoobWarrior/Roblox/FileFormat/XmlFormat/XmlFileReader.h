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
// File: XmlFileReader.h
// Started by: Hattozo
// Started on: 11/3/2025
// Description: This file is derived from Roblox-File-Format (https://github.com/MaximumADHD/Roblox-File-Format/blob/main/XmlFormat/XmlFileReader.cs)
#pragma once
#include <NoobWarrior/Roblox/FileFormat/Tree/Instance.h>

#include <pugixml.hpp>

#include <memory>

namespace NoobWarrior::Roblox {
enum class XmlReadResponse {
    Failed,
    Success,
    InvalidMetadataNode,
    InvalidPropertyNode,
    InvalidItemNode
};

class XmlRobloxFile;
class XmlRobloxFileReader {
public:
    static XmlReadResponse ReadMetadata(pugi::xml_node &meta, XmlRobloxFile *file);
    static XmlReadResponse ReadProperties(Instance &inst, pugi::xml_node &propsNode);
    static XmlReadResponse ReadInstance(Instance** inst, pugi::xml_node &instNode, XmlRobloxFile *file);
};
}