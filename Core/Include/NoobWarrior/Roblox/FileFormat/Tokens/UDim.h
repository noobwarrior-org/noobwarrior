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
// File: UDim.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: UDim property token.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/Interfaces/IXmlPropertyToken.h>
#include <NoobWarrior/Roblox/FileFormat/Utility/Formatting.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/DataTypes.h>

namespace NoobWarrior::Roblox::Tokens {
using namespace NoobWarrior::Roblox::DataTypes;

class UDimToken : public IXmlPropertyToken {
public:
    std::string_view XmlPropertyToken() const override { return "UDim"; }

    bool ReadProperty(Property &property, const pugi::xml_node &node) const override {
        if (!node.child("S") && !node.child("O"))
            return false;
        property.Type = PropertyType::UDim;
        property.Value = UDim(ChildFloat(node, "S"), node.child("O").text().as_int());
        return true;
    }

    void WriteProperty(const Property &property, pugi::xml_node node) const override {
        if (const auto *value = property.CastValue<UDim>()) {
            WriteChildFloat(node, "S", value->Scale);
            node.append_child("O").text().set(std::to_string(value->Offset).c_str());
        }
    }
};
}
