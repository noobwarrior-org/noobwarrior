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
// File: BrickColor.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: BrickColor property token.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/Interfaces/IXmlPropertyToken.h>
#include <NoobWarrior/Roblox/FileFormat/Utility/Formatting.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/DataTypes.h>

namespace NoobWarrior::Roblox::Tokens {
using namespace NoobWarrior::Roblox::DataTypes;

class BrickColorToken : public IXmlPropertyToken {
public:
    std::string_view XmlPropertyToken() const override { return "BrickColor"; }

    // The value is the bare palette number rather than a DataTypes::BrickColor: the binary
    // reader stores int32 for a BrickColor column (Chunks/PROP.cpp), the writer refuses anything
    // else, and CoerceToColumnType only converts between integral types -- so a DataTypes value
    // arriving from an .rbxmx would make the whole PROP chunk fail to write. RobloxFiles has the
    // same shape on the binary side; its XML token only wraps the int because C# needs a type to
    // hang the BrickColorId enum off (Tokens/BrickColor.cs:9-11).
    bool ReadProperty(Property &property, const pugi::xml_node &node) const override {
        int32_t number = 0;
        if (!ParseInteger(node.text().as_string(), number))
            return false;
        property.Type = PropertyType::BrickColor;
        property.Value = number;
        return true;
    }

    void WriteProperty(const Property &property, pugi::xml_node node) const override {
        if (const auto *value = property.CastValue<int32_t>())
            node.text().set(std::to_string(*value).c_str());
    }
};
}
