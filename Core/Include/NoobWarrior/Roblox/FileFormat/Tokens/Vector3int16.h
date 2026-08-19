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
// File: Vector3int16.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Vector3int16 property token.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/Interfaces/IXmlPropertyToken.h>
#include <NoobWarrior/Roblox/FileFormat/Utility/Formatting.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/DataTypes.h>

namespace NoobWarrior::Roblox::Tokens {
using namespace NoobWarrior::Roblox::DataTypes;

class Vector3int16Token : public IXmlPropertyToken {
public:
    std::string_view XmlPropertyToken() const override { return "Vector3int16"; }

    bool ReadProperty(Property &property, const pugi::xml_node &node) const override {
        property.Type = PropertyType::Vector3int16;
        property.Value = Vector3int16(
            static_cast<int16_t>(node.child("X").text().as_int()),
            static_cast<int16_t>(node.child("Y").text().as_int()),
            static_cast<int16_t>(node.child("Z").text().as_int()));
        return true;
    }

    void WriteProperty(const Property &property, pugi::xml_node node) const override {
        if (const auto *value = property.CastValue<Vector3int16>()) {
            node.append_child("X").text().set(std::to_string(value->X).c_str());
            node.append_child("Y").text().set(std::to_string(value->Y).c_str());
            node.append_child("Z").text().set(std::to_string(value->Z).c_str());
        }
    }
};
}
