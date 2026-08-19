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
// File: Color3uint8.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Color3uint8 property token, stored as a packed integer.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/Interfaces/IXmlPropertyToken.h>
#include <NoobWarrior/Roblox/FileFormat/Utility/Formatting.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/DataTypes.h>

namespace NoobWarrior::Roblox::Tokens {
using namespace NoobWarrior::Roblox::DataTypes;

class Color3uint8Token : public IXmlPropertyToken {
public:
    std::string_view XmlPropertyToken() const override { return "Color3uint8"; }

    bool ReadProperty(Property &property, const pugi::xml_node &node) const override {
        uint32_t packed = 0;
        if (!ParseInteger(node.text().as_string(), packed))
            return false;
        property.Type = PropertyType::Color3uint8;
        property.Value = Color3uint8(static_cast<uint8_t>((packed >> 16) & 0xff),
                                     static_cast<uint8_t>((packed >> 8) & 0xff),
                                     static_cast<uint8_t>(packed & 0xff));
        return true;
    }

    void WriteProperty(const Property &property, pugi::xml_node node) const override {
        if (const auto *value = property.CastValue<Color3uint8>()) {
            // The alpha byte is always 0xFF (Tokens/Color3uint8.cs:39). Leaving it out makes
            // white serialize as 16777215 instead of 4294967295, which the reader then sees as a
            // different value than the one Roblox itself writes.
            const uint32_t packed = (0xffu << 24) | (static_cast<uint32_t>(value->R) << 16) |
                (static_cast<uint32_t>(value->G) << 8) | value->B;
            node.text().set(std::to_string(packed).c_str());
        }
    }
};
}
