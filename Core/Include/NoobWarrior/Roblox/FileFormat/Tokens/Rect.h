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
// File: Rect.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Rect property token, written as <Rect2D>.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/Interfaces/IXmlPropertyToken.h>
#include <NoobWarrior/Roblox/FileFormat/Utility/Formatting.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/DataTypes.h>

namespace NoobWarrior::Roblox::Tokens {
using namespace NoobWarrior::Roblox::DataTypes;

class RectToken : public IXmlPropertyToken {
public:
    std::string_view XmlPropertyToken() const override { return "Rect2D"; }

    bool ReadProperty(Property &property, const pugi::xml_node &node) const override {
        const pugi::xml_node min = node.child("min");
        const pugi::xml_node max = node.child("max");
        if (!min || !max)
            return false;
        property.Type = PropertyType::Rect;
        property.Value = Rect({ChildFloat(min, "X"), ChildFloat(min, "Y")},
                              {ChildFloat(max, "X"), ChildFloat(max, "Y")});
        return true;
    }

    void WriteProperty(const Property &property, pugi::xml_node node) const override {
        if (const auto *value = property.CastValue<Rect>()) {
            pugi::xml_node min = node.append_child("min");
            WriteChildFloat(min, "X", value->Min.X);
            WriteChildFloat(min, "Y", value->Min.Y);
            pugi::xml_node max = node.append_child("max");
            WriteChildFloat(max, "X", value->Max.X);
            WriteChildFloat(max, "Y", value->Max.Y);
        }
    }
};
}
