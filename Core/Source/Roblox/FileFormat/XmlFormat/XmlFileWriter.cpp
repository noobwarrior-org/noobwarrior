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
// File: XmlFileWriter.cpp
// Started by: Hattozo
// Started on: 8/19/2026
// Description: This file is derived from Roblox-File-Format (https://github.com/MaximumADHD/Roblox-File-Format/blob/main/XmlFormat/XmlFileWriter.cs)
#include <NoobWarrior/Roblox/FileFormat/XmlFormat/XmlFileWriter.h>

#include <NoobWarrior/Log.h>

#include <cstddef>
#include <string>
#include <vector>

using namespace NoobWarrior::Roblox;
using namespace NoobWarrior::Roblox::XmlFormat;

namespace {
// Instance does not carry Archivable as a field, so it is read back out of the property map.
// An instance that never had the property is archivable, which is the class default.
bool IsArchivable(const Instance &instance) {
    return instance.GetPropertyValue<bool>("Archivable", true);
}

// XmlFileWriter.cs:58-116. A property assembled in memory carries no XmlToken -- nothing outside
// the reader ever sets one -- so the element name has to come from the property's type instead.
// Writing such a property as <string> makes StringToken's cast miss and emit an empty element,
// which destroys the value rather than merely mislabelling it.
std::string XmlTokenForType(const Property &property) {
    switch (property.Type) {
    // XmlFileWriter.cs:115-116 forces Ref no matter what the property claimed.
    case PropertyType::Ref: return "Ref";
    case PropertyType::CFrame:
    case PropertyType::Quaternion: return "CoordinateFrame";
    // Not one of the reference's cases: RobloxFiles would derive the enumerator name
    // "OptionalCFrame", which no token owns, and drop the property. OptionalCFrameToken names
    // the element the engines actually read.
    case PropertyType::OptionalCFrame: return "OptionalCoordinateFrame";
    case PropertyType::Enum: return "token";
    case PropertyType::Rect: return "Rect2D";
    // XmlFileWriter.cs:90-98 lowercases the primitive enumerator names.
    case PropertyType::Int: return "int";
    case PropertyType::Bool: return "bool";
    case PropertyType::Float: return "float";
    case PropertyType::Int64: return "int64";
    case PropertyType::Double: return "double";
    // Same reasoning as OptionalCFrame: the enumerator is FontFace, the element is Font, and
    // PluginTreeMaterializer builds FontFace properties with no XmlToken on them.
    case PropertyType::FontFace: return "Font";
    case PropertyType::String:
        // XmlFileWriter.cs:99-111 picks the element from the value's runtime type, because
        // RobloxFiles files all four of these under PropertyType.String.
        if (property.CastValue<DataTypes::ContentId>() != nullptr) return "Content";
        if (property.CastValue<DataTypes::ProtectedString>() != nullptr) return "ProtectedString";
        if (property.CastValue<std::vector<unsigned char>>() != nullptr) return "BinaryString";
        return "string";
    // The rest are named after their DataType, which is what the enumerator is called.
    case PropertyType::ProtectedString: return "ProtectedString";
    case PropertyType::Content: return "Content";
    case PropertyType::SharedString: return "SharedString";
    case PropertyType::BrickColor: return "BrickColor";
    case PropertyType::UDim: return "UDim";
    case PropertyType::UDim2: return "UDim2";
    case PropertyType::Ray: return "Ray";
    case PropertyType::Faces: return "Faces";
    case PropertyType::Axes: return "Axes";
    case PropertyType::Color3: return "Color3";
    case PropertyType::Color3uint8: return "Color3uint8";
    case PropertyType::Vector2: return "Vector2";
    case PropertyType::Vector3: return "Vector3";
    case PropertyType::Vector3int16: return "Vector3int16";
    case PropertyType::NumberSequence: return "NumberSequence";
    case PropertyType::ColorSequence: return "ColorSequence";
    case PropertyType::NumberRange: return "NumberRange";
    case PropertyType::PhysicalProperties: return "PhysicalProperties";
    case PropertyType::UniqueId: return "UniqueId";
    case PropertyType::SecurityCapabilities: return "SecurityCapabilities";
    case PropertyType::Unknown: break;
    }
    // No token owns "Unknown", so the caller logs and drops it instead of guessing an element.
    return "Unknown";
}

// BinaryString.cs:31-46 writes a payload as base64 wrapped at 72 columns inside a CDATA section.
// An empty payload gets no child at all, which is what the reference is left with after it
// strips "<![CDATA[]]>" back out of the document (XmlRobloxFile.cs:187-188).
void AppendBase64Cdata(pugi::xml_node node, const std::string &base64) {
    if (base64.empty())
        return;
    std::string wrapped;
    wrapped.reserve(base64.size() + base64.size() / 72 + 1);
    for (size_t offset = 0; offset < base64.size(); offset += 72) {
        if (offset > 0)
            wrapped += '\n';
        wrapped += base64.substr(offset, 72);
    }
    node.append_child(pugi::node_cdata).set_value(wrapped.c_str());
}

// XmlFileWriter.cs:126-143 records the key of every SharedString property it writes, so the
// table at the end of the document lists exactly the strings still in use.
void RecordSharedString(const XmlRobloxFile &file, XmlRobloxFile::SharedStringTable &table,
                        const std::string &key) {
    if (key.empty())
        return;
    for (const auto &entry : table) {
        if (entry.first == key)
            return;
    }
    // SharedStringToken carries only the key, so the payload comes from the table the document
    // was read with. A key with no payload is still listed, which is what SharedString.Find
    // returning null leaves behind (XmlFileWriter.cs:232-233).
    std::string payload;
    for (const auto &entry : file.SharedStrings) {
        if (entry.first == key) {
            payload = entry.second;
            break;
        }
    }
    table.emplace_back(key, payload);
}
} // namespace

pugi::xml_node XmlRobloxFileWriter::WriteProperty(
    const Property &property, pugi::xml_node propertiesNode, const XmlRobloxFile &file,
    XmlRobloxFile::SharedStringTable &sharedStrings) {
    // Archivable steers serialization and is never itself serialized (XmlFileWriter.cs:55-56).
    if (property.Name == "Archivable")
        return {};

    // A property read out of a file names its own element; one built in memory has to have the
    // element derived from its type, or the value is written away empty. A Ref is the one type
    // whose element is forced rather than trusted (XmlFileWriter.cs:115-116).
    const std::string element = property.Type == PropertyType::Ref ? "Ref"
        : property.XmlToken.empty() ? XmlTokenForType(property) : property.XmlToken;
    const Tokens::IXmlPropertyToken *token = FindToken(element);
    if (token == nullptr) {
        // XmlFileWriter.cs:120-124 logs and drops. Falling back to "string" would write an
        // element the value cannot be read back out of, which is worse than losing it here.
        const RbxObject *owner = property.Object;
        NoobWarrior::Out("XmlRobloxFile",
                         "No token handler found for property type: {} (on {}.{})",
                         element, owner != nullptr ? owner->ClassName : std::string(),
                         property.Name);
        return {};
    }

    pugi::xml_node value = propertiesNode.append_child(element.c_str());
    value.append_attribute("name").set_value(property.Name.c_str());
    token->WriteProperty(property, value);

    if (property.Type == PropertyType::SharedString) {
        if (const auto *shared = property.CastValue<DataTypes::SharedString>())
            RecordSharedString(file, sharedStrings, shared->Key);
    }
    return value;
}

pugi::xml_node XmlRobloxFileWriter::WriteInstance(
    const Instance &instance, pugi::xml_node parent, const XmlRobloxFile &file,
    XmlRobloxFile::SharedStringTable &sharedStrings) {
    // A non-archivable instance keeps its whole subtree out of the file (XmlFileWriter.cs:163,
    // 207-214 and XmlRobloxFile.cs:165, which check it at every level).
    if (!IsArchivable(instance))
        return {};

    pugi::xml_node item = parent.append_child("Item");
    item.append_attribute("class").set_value(instance.ClassName.c_str());
    if (!instance.Referent.empty())
        item.append_attribute("referent").set_value(instance.Referent.c_str());

    pugi::xml_node properties = item.append_child("Properties");
    // TODO: XmlFileWriter.cs:182-196 also skips a property whose value equals the class default.
    // That needs PropertyDescriptor to expose the default for a class/property pair, which is
    // being generated separately; until it lands, every property that has a token is written.
    bool wroteName = false;
    for (const auto &[name, property] : instance.GetProperties()) {
        if (!WriteProperty(property, properties, file, sharedStrings))
            continue;
        if (name == "Name")
            wroteName = true;
    }

    // The reference gets Name from the instance itself (RefreshProperties) and writes it even
    // when it matches the class default (XmlFileWriter.cs:196). Nothing populates a Name
    // property on a tree built in memory, so without this a mounted instance loses its name.
    if (!wroteName) {
        pugi::xml_node value = properties.append_child("string");
        value.append_attribute("name").set_value("Name");
        Tokens::SetText(value, instance.Name);
    }

    // Written in document order, the way XmlFileWriter.cs:207 walks GetChildren(). Tree/Instance.h
    // keeps children in an insertion-ordered vector now, so the referent sort this used to do only
    // scrambled them: a place file's referents are GUIDs, so sorting reordered every subtree.
    for (const Instance *child : instance.GetChildren())
        WriteInstance(*child, item, file, sharedStrings);

    return item;
}

pugi::xml_node XmlRobloxFileWriter::WriteSharedStrings(
    pugi::xml_node parent, const XmlRobloxFile::SharedStringTable &sharedStrings) {
    pugi::xml_node shared = parent.append_child("SharedStrings");
    for (const auto &[key, payload] : sharedStrings) {
        pugi::xml_node entry = shared.append_child("SharedString");
        entry.append_attribute("md5").set_value(key.c_str());
        AppendBase64Cdata(entry, payload);
    }
    return shared;
}
