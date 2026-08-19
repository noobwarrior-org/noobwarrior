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
// File: IXmlPropertyToken.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Reads and writes one property type's XML representation.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/Tree/Property.h>

#include <pugixml.hpp>

#include <string_view>

namespace NoobWarrior::Roblox::Tokens {
class IXmlPropertyToken {
public:
    virtual ~IXmlPropertyToken() = default;

    // The XML element name this token owns, e.g. "Vector3" or "token".
    virtual std::string_view XmlPropertyToken() const = 0;

    // Returns false when the node does not hold a well-formed value, leaving the property alone.
    virtual bool ReadProperty(Property &property, const pugi::xml_node &node) const = 0;
    virtual void WriteProperty(const Property &property, pugi::xml_node node) const = 0;
};
}
