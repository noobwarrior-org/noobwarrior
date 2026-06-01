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
// File: BrickColor.cpp
// Started by: Hattozo
// Started on: 6/1/2026
// Description:
#include <NoobWarrior/Roblox/DataType/BrickColor.h>

using namespace NoobWarrior::Roblox;

const BrickColor::Entry BrickColor::Palette[75] = {
    {"Dirt brown","#564236"}, {"Reddish brown","#694028"}, {"Brown","#7C5C46"},
    {"Sand red","#957977"}, {"Linen","#AF9483"}, {"Burlap","#C7AC78"},
    {"Brick yellow","#D7C59A"}, {"Medium red","#DA867A"}, {"Dusty Rose","#A34B4B"},
    {"CGA brown","#AA5500"}, {"Dark orange","#A05F35"}, {"Nougat","#CC8E69"},
    {"Light orange","#EAB892"}, {"Pastel brown","#FFCC99"}, {"Neon orange","#D5733D"},
    {"Bright orange","#DA8541"}, {"Br. yellowish orange","#E29B40"}, {"Deep orange","#FFAF00"},
    {"Bright yellow","#F5CD30"}, {"Daisy orange","#F8D96D"}, {"Cool yellow","#FDEA8D"},
    {"Earth green","#27462D"}, {"Camo","#3A7D15"}, {"Dark green","#287F47"},
    {"Bright green","#4B974B"}, {"Shamrock","#5B9A4C"}, {"Moss","#7C9C6B"},
    {"Br. yellowish green","#A4BD47"}, {"Navy blue","#002060"}, {"Deep blue","#2154B9"},
    {"Really blue","#0000FF"}, {"Bright blue","#0D69AC"}, {"Steel blue","#527CAE"},
    {"Medium blue","#6E99CA"}, {"Light blue","#B4D2E4"}, {"Bright bluish green","#008F9C"},
    {"Teal","#12EED4"}, {"Pastel blue-green","#9FF3E9"}, {"Toothpaste","#00FFFF"},
    {"Cyan","#04AFEC"}, {"Pastel Blue","#80BBDC"}, {"Pastel light blue","#AFDDFF"},
    {"Bright violet","#6B327C"}, {"Lavender","#8C5B9F"}, {"Lilac","#A75E9B"},
    {"Magenta","#AA00AA"}, {"Royal purple","#6225D1"}, {"Alder","#B480FF"},
    {"Pastel violet","#B1A7FF"}, {"Bright red","#C4281C"}, {"Really red","#FF0000"},
    {"Hot pink","#FF00BF"}, {"Pink","#FF66CC"}, {"Carnation pink","#FF98DC"},
    {"Light reddish violet","#E8BAC8"}, {"Pastel orange","#FFC9C9"}, {"Dark taupe","#5A4C42"},
    {"Cork","#BC9B5D"}, {"Olive","#C1BE42"}, {"Medium green","#A1C48C"},
    {"Grime","#7F8E64"}, {"Sand green","#789082"}, {"Sand blue","#74869D"},
    {"Lime green","#00FF00"}, {"Pastel green","#CCFFCC"}, {"New Yeller","#FFFF00"},
    {"Pastel yellow","#FFFFCC"}, {"Really black","#111111"}, {"Black","#1B2A35"},
    {"Dark stone grey","#635F62"}, {"Medium stone grey","#A3A2A5"}, {"Mid gray","#CDCDCD"},
    {"Light stone grey","#E5E4DF"}, {"White","#F2F3F3"}, {"Institutional white","#F8F8F8"},
};

BrickColor::BrickColor(const std::string &name) : _color(Color3::fromRGB(255, 255, 255))
{
    _name = name;
    _color = Color3(0, 0, 0);

    _r = _color.R;
    _g = _color.G;
    _b = _color.B;
}

BrickColor::~BrickColor()
{

}

std::string BrickColor::Name()
{
    return _name;
}

Color3 BrickColor::Color()
{
    return _color;
}

double BrickColor::R()
{
    return _r;
}

double BrickColor::G()
{
    return _g;
}

double BrickColor::B()
{
    return _b;
}
