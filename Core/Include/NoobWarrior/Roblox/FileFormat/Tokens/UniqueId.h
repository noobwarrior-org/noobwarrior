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
// File: UniqueId.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: UniqueId property token, stored as 32 hex characters.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/Interfaces/IXmlPropertyToken.h>
#include <NoobWarrior/Roblox/FileFormat/Utility/Formatting.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/DataTypes.h>

namespace NoobWarrior::Roblox::Tokens {
using namespace NoobWarrior::Roblox::DataTypes;

class UniqueIdToken : public IXmlPropertyToken {
public:
    std::string_view XmlPropertyToken() const override { return "UniqueId"; }

    bool ReadProperty(Property &property, const pugi::xml_node &node) const override {
        const std::string hex = node.text().as_string();
        if (hex.size() != 32)
            return false;
        // The 32 hex characters are stored most-significant first, so the bytes reverse.
        std::array<unsigned char, 16> bytes {};
        for (size_t index = 0; index < 16; ++index) {
            uint32_t byte = 0;
            if (!ParseInteger(std::string_view(hex).substr(index * 2, 2), byte, 16))
                return false;
            bytes[15 - index] = static_cast<unsigned char>(byte);
        }
        int64_t random = 0;
        uint32_t time = 0;
        uint32_t counter = 0;
        std::memcpy(&random, bytes.data() + 8, sizeof(random));
        std::memcpy(&time, bytes.data() + 4, sizeof(time));
        std::memcpy(&counter, bytes.data(), sizeof(counter));
        property.Type = PropertyType::UniqueId;
        property.Value = UniqueId(random, time, counter);
        return true;
    }

    void WriteProperty(const Property &property, pugi::xml_node node) const override {
        if (const auto *value = property.CastValue<UniqueId>())
            node.text().set(value->ToString().c_str());
    }
};
}
