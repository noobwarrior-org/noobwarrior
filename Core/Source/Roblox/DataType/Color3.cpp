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
// File: Color3.cpp
// Started by: Hattozo
// Started on: 6/1/2026
// Description:
#include <NoobWarrior/Roblox/DataType/Color3.h>

using namespace NoobWarrior::Roblox;

Color3::Color3(double red, double green, double blue)
{
    R = red;
    G = green;
    B = blue;
}

Color3::~Color3()
{

}

Color3 Color3::fromRGB(double red, double green, double blue)
{
    return Color3(red / 255, green / 255, blue / 255);
}

Color3 Color3::toRGB()
{
    return Color3(R * 255, G * 255, B * 255);
}