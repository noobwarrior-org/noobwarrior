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
// File: Content.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Content property token.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/Interfaces/IXmlPropertyToken.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/ContentId.h>
#include <NoobWarrior/Roblox/FileFormat/Utility/Formatting.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/DataTypes.h>

#include <string>

namespace NoobWarrior::Roblox::Tokens {
using namespace NoobWarrior::Roblox::DataTypes;

class ContentToken : public IXmlPropertyToken {
public:
    std::string_view XmlPropertyToken() const override { return "Content"; }

    bool ReadProperty(Property &property, const pugi::xml_node &node) const override {
        // Only <uri> and <Ref> are the new Content datatype. <url>, <binary>, <hash> and <null>
        // are a ContentId, which serializes as a String -- see ContentIdToken.
        const pugi::xml_node child = node.first_child();
        const std::string_view kind = child ? std::string_view(child.name()) : std::string_view();
        if (kind != "uri" && kind != "Ref") {
            static const ContentIdToken kContentId;
            return kContentId.ReadProperty(property, node);
        }

        property.Type = PropertyType::Content;
        if (kind == "uri") {
            property.Value = Content(child.text().as_string());
        } else {
            // Content.cs:34-35 hands the referent *text* to Content(file, refId), because an XML
            // referent is a string ("RBX1A2B3C"). It is kept as a string here and matched to an
            // instance by XmlRobloxFile::ResolveReferences, exactly as RefToken's value is.
            property.Value = Content::FromRefId(child.text().as_string());
        }
        return true;
    }

    void WriteProperty(const Property &property, pugi::xml_node node) const override {
        const auto *value = property.CastValue<Content>();
        if (value == nullptr) {
            // A ContentId came back through this element name, which is what it was read from.
            static const ContentIdToken kContentId;
            kContentId.WriteProperty(property, node);
            return;
        }
        if (value->SourceType == ContentSourceType::Uri) {
            node.append_child("uri").text().set(value->Uri.c_str());
        } else if (value->SourceType == ContentSourceType::Object) {
            // Content.cs:71-72 writes the target's Instance.Referent, a string. A value read from
            // XML already carries that string; one read from a binary file carries the object
            // table index instead, and BinaryRobloxFile spells a binary instance's referent as
            // exactly that index in decimal, so printing it produces the same name the <Item>
            // elements are written with.
            pugi::xml_node referent = node.append_child("Ref");
            if (!value->RefId.empty())
                referent.text().set(value->RefId.c_str());
            else if (value->ObjectReferent >= 0)
                referent.text().set(std::to_string(value->ObjectReferent).c_str());
            else
                referent.text().set("null"); // Ref.cs:21-27 spells "points at nothing" this way.
        } else {
            node.append_child("null");
        }
    }
};
}
