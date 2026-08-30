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
// File: Color3.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Color3 property token.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/Interfaces/IXmlPropertyToken.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/Color3uint8.h>
#include <NoobWarrior/Roblox/FileFormat/Utility/Formatting.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/DataTypes.h>

namespace NoobWarrior::Roblox::Tokens {
using namespace NoobWarrior::Roblox::DataTypes;

class Color3Token : public IXmlPropertyToken {
public:
    std::string_view XmlPropertyToken() const override { return "Color3"; }

    bool ReadProperty(Property &property, const pugi::xml_node &node) const override {
        for (const char *field : {"R", "G", "B"}) {
            if (!node.child(field)) {
                // Roblox packs a Color3 into a single integer when the value came from a
                // Color3uint8, so a node with no channels is not malformed -- Color3.cs:70-75
                // falls back to that token. Returning false instead makes the writer emit a bare
                // <Color3 name="X"/> and the colour is gone.
                static const Color3uint8Token kColor3uint8;
                return kColor3uint8.ReadProperty(property, node);
            }
        }
        property.Type = PropertyType::Color3;
        // Qualified: bare Color3 is ambiguous in TUs that also see the emulator's Roblox::Color3.
        property.Value = DataTypes::Color3(ChildFloat(node, "R"), ChildFloat(node, "G"), ChildFloat(node, "B"));
        return true;
    }

    void WriteProperty(const Property &property, pugi::xml_node node) const override {
        if (const auto *value = property.CastValue<DataTypes::Color3>()) {
            WriteChildFloat(node, "R", value->R);
            WriteChildFloat(node, "G", value->G);
            WriteChildFloat(node, "B", value->B);
        }
    }
};
}
