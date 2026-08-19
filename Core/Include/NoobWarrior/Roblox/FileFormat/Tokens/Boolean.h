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
// File: Boolean.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Boolean property token.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/Interfaces/IXmlPropertyToken.h>
#include <NoobWarrior/Roblox/FileFormat/Utility/Formatting.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/DataTypes.h>

namespace NoobWarrior::Roblox::Tokens {
using namespace NoobWarrior::Roblox::DataTypes;

class BooleanToken : public IXmlPropertyToken {
public:
    std::string_view XmlPropertyToken() const override { return "bool"; }

    bool ReadProperty(Property &property, const pugi::xml_node &node) const override {
        // Boolean.cs:16 goes through bool.Parse, which trims and ignores case; requiring exact
        // lowercase makes a hand-edited "True" fail to read, and a failed read drops the property
        // silently rather than surfacing an error.
        std::string_view text = node.text().as_string();
        const auto space = [](char c) {
            return c == ' ' || c == '\t' || c == '\r' || c == '\n';
        };
        while (!text.empty() && space(text.front()))
            text.remove_prefix(1);
        while (!text.empty() && space(text.back()))
            text.remove_suffix(1);
        const auto equalsIgnoringCase = [text](std::string_view lowercase) {
            if (text.size() != lowercase.size())
                return false;
            for (size_t index = 0; index < text.size(); ++index) {
                const char c = text[index];
                const char lowered = (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
                if (lowered != lowercase[index])
                    return false;
            }
            return true;
        };
        if (!equalsIgnoringCase("true") && !equalsIgnoringCase("false"))
            return false;
        property.Type = PropertyType::Bool;
        property.Value = equalsIgnoringCase("true");
        return true;
    }

    void WriteProperty(const Property &property, pugi::xml_node node) const override {
        if (const auto *value = property.CastValue<bool>())
            node.text().set(*value ? "true" : "false");
    }
};
}
