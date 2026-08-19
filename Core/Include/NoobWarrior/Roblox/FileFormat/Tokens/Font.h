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
// File: Font.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: FontFace property token, written as <Font>.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/Interfaces/IXmlPropertyToken.h>
#include <NoobWarrior/Roblox/FileFormat/Utility/Formatting.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/DataTypes.h>

namespace NoobWarrior::Roblox::Tokens {
using namespace NoobWarrior::Roblox::DataTypes;

class FontToken : public IXmlPropertyToken {
public:
    std::string_view XmlPropertyToken() const override { return "Font"; }

    bool ReadProperty(Property &property, const pugi::xml_node &node) const override {
        // Family and CachedFaceId are each wrapped in a <url> element (or <null/> when empty),
        // so their text sits one level down -- Font.cs:56-76 writes both that way.
        const pugi::xml_node family = node.child("Family");
        const pugi::xml_node familyUrl = family.child("url");
        const pugi::xml_node cached = node.child("CachedFaceId");
        const pugi::xml_node cachedUrl = cached.child("url");
        uint32_t weight = 400;
        ParseInteger(node.child("Weight").text().as_string(), weight);
        const std::string style = node.child("Style").text().as_string();
        property.Type = PropertyType::FontFace;
        property.Value = FontFace(
            familyUrl ? familyUrl.text().as_string() : family.text().as_string(),
            static_cast<FontWeight>(weight),
            style == "Italic" ? FontStyle::Italic : FontStyle::Normal,
            cachedUrl ? cachedUrl.text().as_string() : cached.text().as_string());
        return true;
    }

    void WriteProperty(const Property &property, pugi::xml_node node) const override {
        const auto *value = property.CastValue<FontFace>();
        if (value == nullptr)
            return;
        // Font.cs:50-76: the wrapper element is <url> when the string has content and <null> when
        // it does not, and CachedFaceId is written unconditionally. Hardcoding <url> for an empty
        // Family, or writing CachedFaceId bare, produces a document this token's own reader
        // cannot read back.
        const auto writeWrapped = [](pugi::xml_node parent, const char *name,
                                     const std::string &text) {
            pugi::xml_node wrapper = parent.append_child(name);
            pugi::xml_node kind = wrapper.append_child(text.empty() ? "null" : "url");
            if (!text.empty())
                kind.text().set(text.c_str());
        };
        writeWrapped(node, "Family", value->Family);
        node.append_child("Weight").text().set(
            std::to_string(static_cast<uint32_t>(value->Weight)).c_str());
        node.append_child("Style").text().set(
            value->Style == FontStyle::Italic ? "Italic" : "Normal");
        writeWrapped(node, "CachedFaceId", value->CachedFaceId);
    }
};
}
