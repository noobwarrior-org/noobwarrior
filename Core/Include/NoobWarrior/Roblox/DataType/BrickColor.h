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
// File: BrickColor.h
// Started by: Hattozo
// Started on: 6/1/2026
// Description:
#pragma once

#include <string>
#include <map>

#include "Color3.h"

namespace NoobWarrior::Roblox {
class BrickColor {
public:
    struct Entry {
        const char* name;
        const char* hex;
    };
    static const Entry Palette[75];

    BrickColor(const std::string &name);
    BrickColor(Color3 color);
    ~BrickColor();

    std::string Name();
    Color3 Color();

    double R();
    double G();
    double B();
private:
    std::string _name;
    Color3 _color;

    double _r;
    double _g;
    double _b;
};
}