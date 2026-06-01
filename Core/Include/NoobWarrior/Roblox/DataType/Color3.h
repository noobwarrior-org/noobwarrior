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
// File: Color3.h
// Started by: Hattozo
// Started on: 6/1/2026
// Description:
#pragma once

namespace NoobWarrior::Roblox {
class Color3 {
public:
    Color3(double red, double green, double blue);
    ~Color3();

    static Color3 fromRGB(double red, double green, double blue);

    Color3 toRGB();

    double R;
    double G;
    double B;
};
}