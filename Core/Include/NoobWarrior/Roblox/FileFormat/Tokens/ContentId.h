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
// File: ContentId.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: ContentId property token.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/Interfaces/IXmlPropertyToken.h>
#include <NoobWarrior/Roblox/FileFormat/Utility/Formatting.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/DataTypes.h>

namespace NoobWarrior::Roblox::Tokens {
using namespace NoobWarrior::Roblox::DataTypes;

class ContentIdToken : public IXmlPropertyToken {
public:
    std::string_view XmlPropertyToken() const override { return "ContentId"; }

    // The element is really named "Content"; ContentId and Content collide, and ContentToken
    // redirects here. A ContentId is a String as far as the format is concerned -- the engines
    // noobWarrior targets have no Content property type at all.
    static std::string InnerText(const pugi::xml_node &node) {
        const pugi::xml_node child = node.first_child();
        if (child && child.type() == pugi::node_element)
            return child.text().as_string();
        return node.text().as_string();
    }

    bool ReadProperty(Property &property, const pugi::xml_node &node) const override {
        property.Type = PropertyType::String;
        property.Value = ContentId(InnerText(node));
        return true;
    }

    void WriteProperty(const Property &property, pugi::xml_node node) const override {
        std::string uri;
        if (const auto *value = property.CastValue<ContentId>())
            uri = value->Uri;
        else if (const auto *value = property.CastValue<std::string>())
            uri = *value;
        else if (const auto *value = property.CastValue<Content>())
            uri = value->Uri;

        if (uri.empty())
            node.append_child("null");
        else
            node.append_child("url").text().set(uri.c_str());
    }
};
}
