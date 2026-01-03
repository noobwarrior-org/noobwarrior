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