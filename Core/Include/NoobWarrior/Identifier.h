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
// File: Identifier.h
// Started by: Hattozo
// Started on: 1/10/2026
// Description:
#pragma once
#include <string>
#include <vector>

namespace NoobWarrior {
enum class ProtocolType {
    Unsupported,
    Http,
    Https,
    Plugin,
    Data,
    RbxAsset,
    RbxThumb
};

class Identifier {
public:
    Identifier(const std::string &str, const std::string &cwd = "");

    ProtocolType GetProtocol();
    std::string GetProtocolString();

    std::string Resolve();
    std::vector<unsigned char> RetrieveData();

    inline operator std::string() const {
        return mStr;
    }

    inline operator const char*() const {
        return mStr.c_str();
    }

    inline operator char*() const {
        return const_cast<char*>(mStr.c_str());
    }

    // inline Identifier& operator =(const std::string &str) {
        // mStr = str;
    // }
protected:
    std::string mStr;
};
}