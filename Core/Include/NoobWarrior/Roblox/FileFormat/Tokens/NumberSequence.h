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
// File: NumberSequence.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: NumberSequence property token, space-separated triples.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/Interfaces/IXmlPropertyToken.h>
#include <NoobWarrior/Roblox/FileFormat/Utility/Formatting.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/DataTypes.h>

namespace NoobWarrior::Roblox::Tokens {
using namespace NoobWarrior::Roblox::DataTypes;

class NumberSequenceToken : public IXmlPropertyToken {
public:
    std::string_view XmlPropertyToken() const override { return "NumberSequence"; }

    bool ReadProperty(Property &property, const pugi::xml_node &node) const override {
        const std::vector<std::string_view> parts =
            SplitOnSpaces(node.text().as_string());
        if (parts.empty() || parts.size() % 3 != 0)
            return false;
        NumberSequence sequence;
        for (size_t index = 0; index < parts.size(); index += 3) {
            float time = 0, value = 0, envelope = 0;
            if (!ParseFloat(parts[index], time) || !ParseFloat(parts[index + 1], value) ||
                !ParseFloat(parts[index + 2], envelope)) {
                return false;
            }
            sequence.Keypoints.emplace_back(time, value, envelope);
        }
        property.Type = PropertyType::NumberSequence;
        property.Value = std::move(sequence);
        return true;
    }

    void WriteProperty(const Property &property, pugi::xml_node node) const override {
        if (const auto *value = property.CastValue<NumberSequence>()) {
            std::string text;
            for (const NumberSequenceKeypoint &keypoint : value->Keypoints) {
                text += FormatFloat(keypoint.Time) + " " + FormatFloat(keypoint.Value) + " " +
                    FormatFloat(keypoint.Envelope) + " ";
            }
            node.text().set(text.c_str());
        }
    }
};
}
