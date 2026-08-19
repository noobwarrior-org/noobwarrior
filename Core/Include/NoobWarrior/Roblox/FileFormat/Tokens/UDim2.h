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
// File: UDim2.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: UDim2 property token.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/Interfaces/IXmlPropertyToken.h>
#include <NoobWarrior/Roblox/FileFormat/Utility/Formatting.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/DataTypes.h>

namespace NoobWarrior::Roblox::Tokens {
using namespace NoobWarrior::Roblox::DataTypes;

class UDim2Token : public IXmlPropertyToken {
public:
    std::string_view XmlPropertyToken() const override { return "UDim2"; }

    bool ReadProperty(Property &property, const pugi::xml_node &node) const override {
        if (!node.child("XS") && !node.child("YS"))
            return false;
        property.Type = PropertyType::UDim2;
        property.Value = UDim2(ChildFloat(node, "XS"), node.child("XO").text().as_int(),
                               ChildFloat(node, "YS"), node.child("YO").text().as_int());
        return true;
    }

    void WriteProperty(const Property &property, pugi::xml_node node) const override {
        if (const auto *value = property.CastValue<UDim2>()) {
            WriteChildFloat(node, "XS", value->X.Scale);
            node.append_child("XO").text().set(std::to_string(value->X.Offset).c_str());
            WriteChildFloat(node, "YS", value->Y.Scale);
            node.append_child("YO").text().set(std::to_string(value->Y.Offset).c_str());
        }
    }
};
}
