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
// Started on: 8/19/2026
// Description: This file is derived from Roblox-File-Format (https://github.com/MaximumADHD/Roblox-File-Format/blob/main/XmlFormat/XmlFileReader.cs)
#pragma once

#include <NoobWarrior/Roblox/FileFormat/XmlFormat/XmlRobloxFile.h>

#include <pugixml.hpp>

#include <string>

namespace NoobWarrior::Roblox::XmlFormat {
// The reference's XmlRobloxFileReader, which is a static class of free-standing readers rather
// than part of the document: each takes the node it is given plus the file the results belong to,
// so a caller holding one <Item> subtree can read it without a whole document around it.
//
// Where the reference throws, these report through the bool/error pair the rest of the port uses.
class XmlRobloxFileReader {
public:
    // Every method below refuses a node whose element name is not the one it reads. XmlRobloxFile
    // dispatches on that name already, but these are public entry points and a caller that hands
    // over the wrong subtree would otherwise get a silently empty result.
    static bool ReadSharedStrings(const pugi::xml_node &sharedStrings, XmlRobloxFile &file,
                                  std::string *error);
    static bool ReadMetadata(const pugi::xml_node &meta, XmlRobloxFile &file,
                             std::string *error);
    static bool ReadProperties(Instance &instance, const pugi::xml_node &propsNode,
                               std::string *error);

    // Returns the Instance the node describes, already owned by @p file and parented to
    // @p parent, or null when the subtree could not be read. @p instances collects every
    // referent the subtree declares, for XmlRobloxFile::ResolveReferences to resolve Refs
    // against once the whole document has been read.
    static Instance *ReadInstance(const pugi::xml_node &instNode, XmlRobloxFile &file,
                                  Instance *parent, XmlRobloxFile::ReferentIndex &instances,
                                  std::string *error);
};
}
