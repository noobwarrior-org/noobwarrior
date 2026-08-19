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
// File: Ray.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Ray property token.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/Interfaces/IXmlPropertyToken.h>
#include <NoobWarrior/Roblox/FileFormat/Utility/Formatting.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/DataTypes.h>

namespace NoobWarrior::Roblox::Tokens {
using namespace NoobWarrior::Roblox::DataTypes;

class RayToken : public IXmlPropertyToken {
public:
    std::string_view XmlPropertyToken() const override { return "Ray"; }

    bool ReadProperty(Property &property, const pugi::xml_node &node) const override {
        const pugi::xml_node origin = node.child("origin");
        const pugi::xml_node direction = node.child("direction");
        if (!origin || !direction)
            return false;
        property.Type = PropertyType::Ray;
        property.Value = Ray(
            {ChildFloat(origin, "X"), ChildFloat(origin, "Y"), ChildFloat(origin, "Z")},
            {ChildFloat(direction, "X"), ChildFloat(direction, "Y"),
             ChildFloat(direction, "Z")});
        return true;
    }

    void WriteProperty(const Property &property, pugi::xml_node node) const override {
        if (const auto *value = property.CastValue<Ray>()) {
            pugi::xml_node origin = node.append_child("origin");
            WriteChildFloat(origin, "X", value->Origin.X);
            WriteChildFloat(origin, "Y", value->Origin.Y);
            WriteChildFloat(origin, "Z", value->Origin.Z);
            pugi::xml_node direction = node.append_child("direction");
            WriteChildFloat(direction, "X", value->Direction.X);
            WriteChildFloat(direction, "Y", value->Direction.Y);
            WriteChildFloat(direction, "Z", value->Direction.Z);
        }
    }
};
}
