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
// File: OptionalCFrame.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: OptionalCFrame property token, written as <OptionalCoordinateFrame>.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/Interfaces/IXmlPropertyToken.h>
#include <NoobWarrior/Roblox/FileFormat/Utility/Formatting.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/DataTypes.h>

namespace NoobWarrior::Roblox::Tokens {
using namespace NoobWarrior::Roblox::DataTypes;

class OptionalCFrameToken : public IXmlPropertyToken {
public:
    std::string_view XmlPropertyToken() const override { return "OptionalCoordinateFrame"; }

    bool ReadProperty(Property &property, const pugi::xml_node &node) const override {
        const pugi::xml_node inner = node.child("CFrame");
        property.Type = PropertyType::OptionalCFrame;
        if (!inner) {
            property.Value = std::optional<CFrame>();
            return true;
        }
        std::array<float, 12> components {};
        static constexpr const char *kFields[12] = {"X", "Y", "Z", "R00", "R01", "R02", "R10", "R11", "R12", "R20", "R21", "R22"};
        for (size_t index = 0; index < components.size(); ++index)
            components[index] = ChildFloat(inner, kFields[index]);
        property.Value = std::optional<CFrame>(CFrame(components));
        return true;
    }

    void WriteProperty(const Property &property, pugi::xml_node node) const override {
        const auto *value = property.CastValue<std::optional<CFrame>>();
        if (value == nullptr || !value->has_value())
            return;
        static constexpr const char *kFields[12] = {"X", "Y", "Z", "R00", "R01", "R02", "R10", "R11", "R12", "R20", "R21", "R22"};
        pugi::xml_node inner = node.append_child("CFrame");
        for (size_t index = 0; index < (*value)->Components.size(); ++index)
            WriteChildFloat(inner, kFields[index], (*value)->Components[index]);
    }
};
}
