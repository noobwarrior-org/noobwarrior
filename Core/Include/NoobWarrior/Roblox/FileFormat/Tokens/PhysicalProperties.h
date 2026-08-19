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
// File: PhysicalProperties.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: PhysicalProperties property token.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/Interfaces/IXmlPropertyToken.h>
#include <NoobWarrior/Roblox/FileFormat/Utility/Formatting.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/DataTypes.h>

namespace NoobWarrior::Roblox::Tokens {
using namespace NoobWarrior::Roblox::DataTypes;

class PhysicalPropertiesToken : public IXmlPropertyToken {
public:
    std::string_view XmlPropertyToken() const override { return "PhysicalProperties"; }

    bool ReadProperty(Property &property, const pugi::xml_node &node) const override {
        property.Type = PropertyType::PhysicalProperties;
        const std::string custom = node.child("CustomPhysics").text().as_string();
        if (custom != "true") {
            property.Value = std::optional<PhysicalProperties>();
            return true;
        }
        // A missing child falls back to 1, not 0: Tokens/PhysicalProperties.cs:28 builds its
        // reader with a fallback of 1f. Zero density or friction would be a physically different
        // part, so the difference is not cosmetic.
        PhysicalProperties value(ChildFloat(node, "Density", 1.0f),
                                 ChildFloat(node, "Friction", 1.0f),
                                 ChildFloat(node, "Elasticity", 1.0f),
                                 ChildFloat(node, "FrictionWeight", 1.0f),
                                 ChildFloat(node, "ElasticityWeight", 1.0f));
        // Not a constructor argument because DataTypes::PhysicalProperties still takes the
        // five-field form; PhysicalProperties.cs:44 reads it as a sixth.
        value.AcousticAbsorption = ChildFloat(node, "AcousticAbsorption", 1.0f);
        value.Flags = 0x01;
        property.Value = std::optional<PhysicalProperties>(value);
        return true;
    }

    void WriteProperty(const Property &property, pugi::xml_node node) const override {
        const auto *value = property.CastValue<std::optional<PhysicalProperties>>();
        if (value == nullptr)
            return;
        if (!value->has_value()) {
            node.append_child("CustomPhysics").text().set("false");
            return;
        }
        node.append_child("CustomPhysics").text().set("true");
        WriteChildFloat(node, "Density", (*value)->Density);
        WriteChildFloat(node, "Friction", (*value)->Friction);
        WriteChildFloat(node, "Elasticity", (*value)->Elasticity);
        WriteChildFloat(node, "FrictionWeight", (*value)->FrictionWeight);
        WriteChildFloat(node, "ElasticityWeight", (*value)->ElasticityWeight);
        // PhysicalProperties.cs:78. Older engines ignore the element they do not know, but
        // dropping it loses the value on every save.
        WriteChildFloat(node, "AcousticAbsorption", (*value)->AcousticAbsorption);
    }
};
}
