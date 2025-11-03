// === noobWarrior ===
// File: XmlFileReader.cpp
// Started by: Hattozo
// Started on: 11/3/2025
// Description: This file is derived from Roblox-File-Format (https://github.com/MaximumADHD/Roblox-File-Format/blob/main/XmlFormat/XmlFileReader.cs)
#include <NoobWarrior/Roblox/FileFormat/XmlFormat/XmlFileReader.h>
#include <NoobWarrior/Roblox/FileFormat/XmlFormat/XmlRobloxFile.h>
#include <NoobWarrior/Log.h>

#include <cstring>

using namespace NoobWarrior::Roblox;

XmlReadResponse XmlRobloxFileReader::ReadMetadata(pugi::xml_node &meta, XmlRobloxFile *file) {
    if (strncmp(meta.name(), "Meta", 4) != 0) {
        Out("XmlRobloxFileReader", "Provided xml_node's name should be 'Meta'!");
        return XmlReadResponse::InvalidMetadataNode;
    }

    pugi::xml_attribute propName = meta.attribute("name");

    if (propName.empty()) {
        Out("XmlRobloxFileReader", "Got a Meta node without a 'name' attribute!");
        return XmlReadResponse::InvalidMetadataNode;
    }

    std::string key = propName.as_string();
    std::string value = meta.text().as_string();

    file->Metadata[key] = value;
    return XmlReadResponse::Success;
}

XmlReadResponse XmlRobloxFileReader::ReadProperties(Instance &inst, pugi::xml_node &propsNode) {
    if (strncmp(propsNode.name(), "Properties", 10) != 0) {
        Out("XmlRobloxFileReader", "Provided xml_node's name should be 'Properties'!");
        return XmlReadResponse::InvalidPropertyNode;
    }

    for (pugi::xml_node &propNode : propsNode.children()) {
        if (propNode.type() == pugi::node_comment)
            continue;

        std::string propType = propNode.name();
        pugi::xml_attribute propName = propNode.attribute("name");

        if (propName.empty()) {
            if (strncmp(propNode.name(), "Item", 4) == 0)
                continue;

            Out("XmlRobloxFileReader", "Got a property node without a 'name' attribute!");
            return XmlReadResponse::InvalidPropertyNode;
        }
    }
    return XmlReadResponse::Failed;
}

XmlReadResponse XmlRobloxFileReader::ReadInstance(Instance** inst, pugi::xml_node &instNode, XmlRobloxFile *file) {
    *inst = nullptr;
    if (strncmp(instNode.name(), "Item", 4) != 0) {
        Out("XmlRobloxFileReader", "Provided xml_node's name should be 'Item'!");
        return XmlReadResponse::InvalidItemNode;
    }

    pugi::xml_attribute classToken = instNode.attribute("class");
    if (classToken.empty()) {
        Out("XmlRobloxFileReader", "Got an Item without a defined 'class' attribute!");
        return XmlReadResponse::InvalidItemNode;
    }

    std::string className = classToken.as_string();
    if (className.empty()) {
        Out("XmlRobloxFileReader", "Got an Item with an empty 'class' attribute!");
        return XmlReadResponse::InvalidItemNode;
    }

    // WIP: Reflection.

    return XmlReadResponse::Failed;
}