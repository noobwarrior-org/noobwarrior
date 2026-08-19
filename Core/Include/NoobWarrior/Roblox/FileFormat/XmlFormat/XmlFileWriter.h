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
// File: XmlFileWriter.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: This file is derived from Roblox-File-Format (https://github.com/MaximumADHD/Roblox-File-Format/blob/main/XmlFormat/XmlFileWriter.cs)
#pragma once

#include <NoobWarrior/Roblox/FileFormat/XmlFormat/XmlRobloxFile.h>

#include <pugixml.hpp>

namespace NoobWarrior::Roblox::XmlFormat {
// The reference's XmlRobloxFileWriter. Its methods build a detached XmlNode and hand it back for
// the caller to append; pugixml has no useful detached node, so each takes the parent to append
// to and returns the node it made -- an empty node where the reference returns null.
class XmlRobloxFileWriter {
public:
    // The reference's XmlWriterSettings (XmlFileWriter.cs:16-23) reduced to the parts pugixml
    // takes. Two spaces rather than the reference's tab, because the .rbxlx files noobWarrior
    // already ships were written with this writer and a change here rewrites all of them.
    static constexpr const char *kIndentChars = "  ";

    // @p file supplies the payload for a SharedString key, which the property itself does not
    // carry; @p sharedStrings collects the keys actually written, for WriteSharedStrings.
    static pugi::xml_node WriteProperty(const Property &property, pugi::xml_node propertiesNode,
                                        const XmlRobloxFile &file,
                                        XmlRobloxFile::SharedStringTable &sharedStrings);

    // Empty when the instance is not Archivable, which drops its whole subtree.
    static pugi::xml_node WriteInstance(const Instance &instance, pugi::xml_node parent,
                                        const XmlRobloxFile &file,
                                        XmlRobloxFile::SharedStringTable &sharedStrings);

    static pugi::xml_node WriteSharedStrings(
        pugi::xml_node parent, const XmlRobloxFile::SharedStringTable &sharedStrings);
};
}
