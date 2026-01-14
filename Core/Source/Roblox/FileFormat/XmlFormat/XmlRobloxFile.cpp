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
// File: XmlRobloxFile.cpp
// Started by: Hattozo
// Started on: 3/9/2025
// Description: This file is derived from Roblox-File-Format (https://github.com/MaximumADHD/Roblox-File-Format/blob/main/XmlFormat/XmlRobloxFile.cs)
#include <NoobWarrior/Roblox/FileFormat/XmlFormat/XmlRobloxFile.h>
#include <NoobWarrior/Roblox/FileFormat/XmlFormat/XmlFileReader.h>
#include <NoobWarrior/Roblox/FileFormat/RobloxFile.h>
#include <NoobWarrior/Log.h>

#include <memory>
#include <cstring>

using namespace NoobWarrior::Roblox;

XmlRobloxFile::XmlRobloxFile() : RefCounter(0) {
    Name = "Xml:";
    Referent = "null";
    ParentLocked = true;
}

FileResponse XmlRobloxFile::ReadFile(std::vector<unsigned char> buffer) {
    // std::string xml;
    // xml.assign(buffer.begin(), buffer.end());
    pugi::xml_parse_result res = XmlDocument.load_buffer(buffer.data(), buffer.size());
    if (res.status != pugi::xml_parse_status::status_ok) {
        return FileResponse::CouldNotParse;
    }
    pugi::xml_node roblox = XmlDocument.select_node("roblox").node();
    if (!roblox.empty() && strncmp(roblox.name(), "roblox", 6) == 0) {
        pugi::xml_attribute version = roblox.attribute("version");

        if (version.empty() || version.as_int(-1) == -1) {
            Out("XmlRobloxFile", "No proper version number defined in xml data");
            return FileResponse::InvalidVersion;
        } else if (version.as_int(-1) < 4) {
            Out("XmlRobloxFile", "Provided version must be at least 4!");
            return FileResponse::VersionTooLow;
        }

        for (pugi::xml_node &childNode : roblox.children()) {
            if (strncmp(childNode.name(), "Item", 4) == 0) {
                Instance* child = nullptr;
                XmlReadResponse res = XmlRobloxFileReader::ReadInstance(&child, childNode, this);

                if (res != XmlReadResponse::Success) {
                    Out("XmlRobloxFile", "Failed to read instance!");
                    continue;
                }

                if (child == nullptr)
                    continue;

                child->SetParent(this);
            }
        }
    }
    return FileResponse::Failed;
}

void XmlRobloxFile::Save() {
    
}