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
// File: ProtectedString.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: ProtectedString property token; Script.Source uses it.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/Interfaces/IXmlPropertyToken.h>
#include <NoobWarrior/Roblox/FileFormat/Utility/Formatting.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/DataTypes.h>

namespace NoobWarrior::Roblox::Tokens {
using namespace NoobWarrior::Roblox::DataTypes;

class ProtectedStringToken : public IXmlPropertyToken {
public:
    std::string_view XmlPropertyToken() const override { return "ProtectedString"; }

    // ProtectedString.cs:16 sets PropertyType.String, because RobloxFiles hands the value on as a
    // plain string. This port keeps the distinct PropertyType::ProtectedString on purpose: the
    // binary path carries it end to end (BinaryRobloxFile's default value, PROP's own
    // ProtectedString column case, PluginTreeMaterializer, XmlRobloxFile's Script.Source shim),
    // and PROP writes that column from DataTypes::ProtectedString::RawBuffer rather than through
    // the String case. Flattening it here would leave the property typed String while still
    // holding a ProtectedString, which only survives because CoerceToColumnType converts between
    // the two -- not a coincidence worth depending on.
    bool ReadProperty(Property &property, const pugi::xml_node &node) const override {
        property.Type = PropertyType::ProtectedString;
        property.Value = ProtectedString(std::string(node.text().as_string()));
        return true;
    }

    void WriteProperty(const Property &property, pugi::xml_node node) const override {
        if (const auto *value = property.CastValue<ProtectedString>())
            SetText(node, value->ToString());
    }
};
}
