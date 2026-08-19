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
// File: CFrame.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: CFrame property token, written as <CoordinateFrame>.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/Interfaces/IXmlPropertyToken.h>
#include <NoobWarrior/Roblox/FileFormat/Utility/Formatting.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/DataTypes.h>

namespace NoobWarrior::Roblox::Tokens {
using namespace NoobWarrior::Roblox::DataTypes;

class CFrameToken : public IXmlPropertyToken {
public:
    std::string_view XmlPropertyToken() const override { return "CoordinateFrame"; }

    bool ReadProperty(Property &property, const pugi::xml_node &node) const override {
        if (!node.child("X"))
            return false;
        std::array<float, 12> components {};
        static constexpr const char *kFields[12] = {"X", "Y", "Z", "R00", "R01", "R02", "R10", "R11", "R12", "R20", "R21", "R22"};
        for (size_t index = 0; index < components.size(); ++index)
            components[index] = ChildFloat(node, kFields[index]);
        property.Type = PropertyType::CFrame;
        property.Value = CFrame(components);
        return true;
    }

    void WriteProperty(const Property &property, pugi::xml_node node) const override {
        if (const auto *value = property.CastValue<CFrame>()) {
            static constexpr const char *kFields[12] = {"X", "Y", "Z", "R00", "R01", "R02", "R10", "R11", "R12", "R20", "R21", "R22"};
            for (size_t index = 0; index < value->Components.size(); ++index)
                WriteChildFloat(node, kFields[index], value->Components[index]);
        }
    }
};
}
