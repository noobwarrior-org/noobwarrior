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
// File: NumberRange.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: NumberRange property token, a space-separated pair.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/Interfaces/IXmlPropertyToken.h>
#include <NoobWarrior/Roblox/FileFormat/Utility/Formatting.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/DataTypes.h>

namespace NoobWarrior::Roblox::Tokens {
using namespace NoobWarrior::Roblox::DataTypes;

class NumberRangeToken : public IXmlPropertyToken {
public:
    std::string_view XmlPropertyToken() const override { return "NumberRange"; }

    bool ReadProperty(Property &property, const pugi::xml_node &node) const override {
        const std::vector<std::string_view> parts =
            SplitOnSpaces(node.text().as_string());
        if (parts.size() != 2)
            return false;
        float min = 0;
        float max = 0;
        if (!ParseFloat(parts[0], min) || !ParseFloat(parts[1], max))
            return false;
        property.Type = PropertyType::NumberRange;
        property.Value = NumberRange(min, max);
        return true;
    }

    void WriteProperty(const Property &property, pugi::xml_node node) const override {
        if (const auto *value = property.CastValue<NumberRange>()) {
            node.text().set((FormatFloat(value->Min) + " " + FormatFloat(value->Max) + " ").c_str());
        }
    }
};
}
