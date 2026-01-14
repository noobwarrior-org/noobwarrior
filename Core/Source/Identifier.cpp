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
// File: Identifier.cpp
// Started by: Hattozo
// Started on: 1/10/2026
// Description:
#include <NoobWarrior/Identifier.h>

#define CHECK_PROTOCOL(str, type) if (protocolStr.compare(str) == 0) return type; // how to mask bad code 101

using namespace NoobWarrior;

Identifier::Identifier(const std::string &str, const std::string &cwd) : mStr(str) {

}

ProtocolType Identifier::GetProtocol() {
    std::string protocolStr = GetProtocolString();
    CHECK_PROTOCOL("http", ProtocolType::Http)
    CHECK_PROTOCOL("https", ProtocolType::Https)
    CHECK_PROTOCOL("plugin", ProtocolType::Plugin)
    CHECK_PROTOCOL("data", ProtocolType::Data)
    CHECK_PROTOCOL("rbxasset", ProtocolType::RbxAsset)
    CHECK_PROTOCOL("rbxthumb", ProtocolType::RbxThumb)
    return ProtocolType::Unsupported;
}

std::string Identifier::GetProtocolString() {
    int pos = mStr.find("://");
    if (pos) {
        std::string protocol = mStr.substr(0 , pos);
        return protocol;
    } else return "";
}