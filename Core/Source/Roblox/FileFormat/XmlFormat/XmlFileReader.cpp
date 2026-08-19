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
// File: XmlFileReader.cpp
// Started by: Hattozo
// Started on: 8/19/2026
// Description: This file is derived from Roblox-File-Format (https://github.com/MaximumADHD/Roblox-File-Format/blob/main/XmlFormat/XmlFileReader.cs)
#include <NoobWarrior/Roblox/FileFormat/XmlFormat/XmlFileReader.h>

#include <cstring>
#include <memory>
#include <optional>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

using namespace NoobWarrior::Roblox;
using namespace NoobWarrior::Roblox::XmlFormat;

namespace {
// The serialized fragment is kept alongside the decoded value: consumers such as
// PluginDataModel re-read a property from its original XML rather than from the typed value.
std::vector<unsigned char> SerializeXmlNode(const pugi::xml_node &node) {
    std::ostringstream stream;
    node.print(stream, "", pugi::format_raw, pugi::encoding_utf8);
    const std::string serialized = stream.str();
    return {serialized.begin(), serialized.end()};
}

// Only the decoded length is wanted: XmlFileReader.cs:41-45 decodes a shared string key purely
// to refuse one that is not exactly 16 bytes, and nothing in the port looks inside the key.
std::optional<size_t> Base64DecodedSize(std::string_view text) {
    if (text.empty() || text.size() % 4 != 0)
        return std::nullopt;
    size_t padding = 0;
    while (padding < 2 && text[text.size() - 1 - padding] == '=')
        ++padding;
    for (size_t index = 0; index + padding < text.size(); ++index) {
        const char character = text[index];
        const bool encoded = (character >= 'A' && character <= 'Z') ||
            (character >= 'a' && character <= 'z') ||
            (character >= '0' && character <= '9') ||
            character == '+' || character == '/';
        if (!encoded)
            return std::nullopt;
    }
    return text.size() / 4 * 3 - padding;
}

// XmlFileReader.cs:39 drops the newlines the payload was wrapped with; pugixml hands back the CR
// of a CRLF-terminated document as well.
std::string StripNewlines(std::string_view text) {
    std::string stripped;
    stripped.reserve(text.size());
    for (const char character : text) {
        if (character != '\n' && character != '\r')
            stripped += character;
    }
    return stripped;
}
} // namespace

bool XmlRobloxFileReader::ReadSharedStrings(const pugi::xml_node &sharedStrings,
                                            XmlRobloxFile &file, std::string *error) {
    if (std::strcmp(sharedStrings.name(), "SharedStrings") != 0) {
        SetError(error, "Provided XmlNode's class must be 'SharedStrings'!");
        return false;
    }

    for (pugi::xml_node shared : sharedStrings.children("SharedString")) {
        const pugi::xml_attribute md5 = shared.attribute("md5");
        if (!md5) {
            SetError(error, "Got a SharedString without an 'md5' attribute");
            return false;
        }
        // The key indexes the table and must be 16 bytes wide, whether or not it is the digest
        // of this payload (XmlFileReader.cs:41-51); anything else is a table no property can
        // address.
        const std::string key = md5.as_string();
        if (Base64DecodedSize(key) != 16u) {
            SetError(error, "SharedString base64 key '" + key + "' must align to byte[16]");
            return false;
        }
        file.SharedStrings.emplace_back(key, StripNewlines(shared.text().as_string()));
    }
    return true;
}

bool XmlRobloxFileReader::ReadMetadata(const pugi::xml_node &meta, XmlRobloxFile &file,
                                       std::string *error) {
    if (std::strcmp(meta.name(), "Meta") != 0) {
        SetError(error, "Provided XmlNode's class should be 'Meta'!");
        return false;
    }
    file.Metadata.emplace_back(meta.attribute("name").as_string(), meta.text().as_string());
    return true;
}

bool XmlRobloxFileReader::ReadProperties(Instance &instance, const pugi::xml_node &propsNode,
                                         std::string *error) {
    if (std::strcmp(propsNode.name(), "Properties") != 0) {
        SetError(error, "Provided XmlNode's class should be 'Properties'!");
        return false;
    }

    for (pugi::xml_node value : propsNode.children()) {
        if (value.type() != pugi::node_element)
            continue;
        const pugi::xml_attribute name = value.attribute("name");
        if (!name)
            continue;

        Property property;
        property.Name = name.as_string();
        property.XmlToken = value.name();
        property.RawBuffer = SerializeXmlNode(value);
        // Assigned before the token reads rather than left to AddProperty afterwards: IntToken
        // asks property.Object for the declaring class so it can tell a BrickColor written as
        // <int> from a plain int, and XmlFileReader.cs:100-104 sets Object in the same
        // initializer that names the property for that reason.
        property.Object = &instance;
        const Tokens::IXmlPropertyToken *token = FindToken(value.name());
        if (token != nullptr)
            token->ReadProperty(property, value);
        if (property.Name == "Name") {
            if (const auto *text = property.CastValue<std::string>())
                instance.Name = *text;
        }
        instance.AddProperty(std::move(property));
    }
    return true;
}

Instance *XmlRobloxFileReader::ReadInstance(const pugi::xml_node &instNode, XmlRobloxFile &file,
                                            Instance *parent,
                                            XmlRobloxFile::ReferentIndex &instances,
                                            std::string *error) {
    if (std::strcmp(instNode.name(), "Item") != 0) {
        SetError(error, "Provided XmlNode's name should be 'Item'!");
        return nullptr;
    }

    const pugi::xml_attribute className = instNode.attribute("class");
    if (!className) {
        SetError(error, "Item node has no class attribute");
        return nullptr;
    }

    auto owned = std::make_unique<Instance>();
    Instance *instance = owned.get();
    instance->ClassName = className.as_string();
    instance->Name = instance->ClassName;
    file.Objects.push_back(std::move(owned));

    // The referent is optional, but it is what a Ref property names, so two Items sharing one
    // makes every Ref that uses it ambiguous and the reference refuses the whole document
    // (XmlFileReader.cs:157-166).
    const pugi::xml_attribute referent = instNode.attribute("referent");
    if (referent) {
        instance->Referent = referent.as_string();
        if (!instances.emplace(instance->Referent, instance).second) {
            SetError(error, "Got an Item with a duplicate 'referent' attribute: " +
                            instance->Referent);
            return nullptr;
        }
    }

    for (pugi::xml_node child : instNode.children()) {
        if (std::strcmp(child.name(), "Properties") == 0) {
            if (!ReadProperties(*instance, child, error))
                return nullptr;
        } else if (std::strcmp(child.name(), "Item") == 0) {
            if (ReadInstance(child, file, instance, instances, error) == nullptr)
                return nullptr;
        }
    }

    if (parent != nullptr && !instance->SetParent(parent)) {
        SetError(error, "Item nodes do not form a valid tree");
        return nullptr;
    }
    return instance;
}
