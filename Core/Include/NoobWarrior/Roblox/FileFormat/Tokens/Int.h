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
// File: Int.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Int property token.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/Interfaces/IXmlPropertyToken.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/BrickColor.h>
#include <NoobWarrior/Roblox/FileFormat/Utility/Formatting.h>
#include <NoobWarrior/Roblox/FileFormat/Utility/ImplicitMember.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/DataTypes.h>
#include <NoobWarrior/Roblox/FileFormat/Tree/RbxObject.h>

namespace NoobWarrior::Roblox::Tokens {
using namespace NoobWarrior::Roblox::DataTypes;

class IntToken : public IXmlPropertyToken {
public:
    std::string_view XmlPropertyToken() const override { return "int"; }

    bool ReadProperty(Property &property, const pugi::xml_node &node) const override {
        // Roblox writes a BrickColor as <int name="BrickColor">194</int>, so nothing in the
        // document distinguishes it from a plain int -- only the declared member type does.
        // Tokens/Int.cs:20-24 does this lookup by reflection; ImplicitMember is the same
        // question asked of the generated API dump. Without it BrickColorToken is unreachable,
        // because no real file uses <BrickColor> as an element name.
        //
        // Int.cs narrows to Type.GetField, i.e. to members the engine still serializes, which
        // would drop BasePart.BrickColor -- the dump marks it CanSave=false because 0.735 saves
        // Color3uint8 instead. Every place old enough to write <int name="BrickColor"> is
        // exactly the case this port has to read, and tagging it PropertyType::Int would send it
        // back out as an Int PROP column that no target engine accepts for a BrickColor
        // property. So the alias counts here, and ImplicitMember::IsSerializedMember is
        // deliberately not consulted.
        if (property.Object != nullptr) {
            using NoobWarrior::Roblox::Utility::ImplicitMember;
            if (ImplicitMember::MemberType(property.Object->ClassName, property.Name) ==
                "BrickColor") {
                static const BrickColorToken kBrickColor;
                return kBrickColor.ReadProperty(property, node);
            }
        }
        int32_t value = 0;
        if (!ParseInteger(node.text().as_string(), value))
            return false;
        property.Type = PropertyType::Int;
        property.Value = value;
        return true;
    }

    void WriteProperty(const Property &property, pugi::xml_node node) const override {
        if (const auto *value = property.CastValue<int32_t>())
            node.text().set(std::to_string(*value).c_str());
    }
};
}
