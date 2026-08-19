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
// File: Axes.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Axes property token, a bitfield.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/Interfaces/IXmlPropertyToken.h>
#include <NoobWarrior/Roblox/FileFormat/Utility/Formatting.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/DataTypes.h>

namespace NoobWarrior::Roblox::Tokens {
using namespace NoobWarrior::Roblox::DataTypes;

class AxesToken : public IXmlPropertyToken {
public:
    std::string_view XmlPropertyToken() const override { return "Axes"; }

    bool ReadProperty(Property &property, const pugi::xml_node &node) const override {
        uint32_t value = 0;
        // The value sits in a nested element in some files and directly in the node in others.
        const pugi::xml_node inner = node.first_child().type() == pugi::node_element
            ? node.first_child() : node;
        if (!ParseInteger(inner.text().as_string(), value))
            return false;
        property.Type = PropertyType::Axes;
        property.Value = Axes(static_cast<uint8_t>(value));
        return true;
    }

    void WriteProperty(const Property &property, pugi::xml_node node) const override {
        if (const auto *value = property.CastValue<Axes>()) {
            node.append_child("axes").text().set(std::to_string(value->Flags()).c_str());
        }
    }
};
}
