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