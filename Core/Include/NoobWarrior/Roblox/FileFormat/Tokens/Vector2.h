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
// File: Vector2.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Vector2 property token.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/Interfaces/IXmlPropertyToken.h>
#include <NoobWarrior/Roblox/FileFormat/Utility/Formatting.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/DataTypes.h>

namespace NoobWarrior::Roblox::Tokens {
using namespace NoobWarrior::Roblox::DataTypes;

class Vector2Token : public IXmlPropertyToken {
public:
    std::string_view XmlPropertyToken() const override { return "Vector2"; }

    bool ReadProperty(Property &property, const pugi::xml_node &node) const override {
        for (const char *field : {"X", "Y"}) {
            if (!node.child(field))
                return false;
        }
        property.Type = PropertyType::Vector2;
        property.Value = Vector2(ChildFloat(node, "X"), ChildFloat(node, "Y"));
        return true;
    }

    void WriteProperty(const Property &property, pugi::xml_node node) const override {
        if (const auto *value = property.CastValue<Vector2>()) {
            WriteChildFloat(node, "X", value->X);
            WriteChildFloat(node, "Y", value->Y);
        }
    }
};
}
