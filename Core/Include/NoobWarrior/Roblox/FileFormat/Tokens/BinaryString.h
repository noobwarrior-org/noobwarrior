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
// File: BinaryString.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: BinaryString property token; the payload is base64 in the XML form.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/Interfaces/IXmlPropertyToken.h>
#include <NoobWarrior/Roblox/FileFormat/Utility/Formatting.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/DataTypes.h>

namespace NoobWarrior::Roblox::Tokens {
using namespace NoobWarrior::Roblox::DataTypes;

class BinaryStringToken : public IXmlPropertyToken {
public:
    std::string_view XmlPropertyToken() const override { return "BinaryString"; }

    bool ReadProperty(Property &property, const pugi::xml_node &node) const override {
        // BinaryString.cs:12-16 decodes before storing. Keeping the base64 text as the value only
        // round-trips through XML: saved to a .rbxl the engine would get the base64 characters
        // where it expects raw bytes (Terrain.SmoothGrid, AttributesSerialize, mesh data).
        std::vector<unsigned char> decoded;
        if (!Base64Decode(node.text().as_string(), decoded))
            return false;
        property.Type = PropertyType::String;
        property.Value = std::string(decoded.begin(), decoded.end());
        return true;
    }

    void WriteProperty(const Property &property, pugi::xml_node node) const override {
        const auto *text = property.CastValue<std::string>();
        if (text == nullptr)
            return;
        // BinaryString.cs:29-46: base64, wrapped every 72 characters, emitted as CDATA.
        const std::string encoded = Base64Encode(*text);
        std::string wrapped;
        wrapped.reserve(encoded.size() + encoded.size() / kLineLength + 1);
        for (size_t index = 0; index < encoded.size(); index += kLineLength) {
            if (index != 0)
                wrapped.push_back('\n');
            wrapped.append(encoded, index, kLineLength);
        }
        node.append_child(pugi::node_cdata).set_value(wrapped.c_str());
    }

private:
    static constexpr size_t kLineLength = 72;
};
}
