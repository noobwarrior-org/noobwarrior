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

#include <cstdlib>

using namespace NoobWarrior::Roblox;

// name, preview hex, canonical Roblox BrickColor number. Numbers verified against the official
// BrickColor palette by matching each entry's RGB; the engine reads these numbers as the avatar's
// headColorId/torsoColorId/etc.
const BrickColor::Entry BrickColor::Palette[75] = {
    {"Dirt brown","#564236",361}, {"Reddish brown","#694028",192}, {"Brown","#7C5C46",217},
    {"Sand red","#957977",153}, {"Linen","#AF9483",359}, {"Burlap","#C7AC78",352},
    {"Brick yellow","#D7C59A",5}, {"Medium red","#DA867A",101}, {"Dusty Rose","#A34B4B",1007},
    {"CGA brown","#AA5500",1014}, {"Dark orange","#A05F35",38}, {"Nougat","#CC8E69",18},
    {"Light orange","#EAB892",125}, {"Pastel brown","#FFCC99",1030}, {"Neon orange","#D5733D",133},
    {"Bright orange","#DA8541",106}, {"Br. yellowish orange","#E29B40",105}, {"Deep orange","#FFAF00",1005},
    {"Bright yellow","#F5CD30",24}, {"Daisy orange","#F8D96D",334}, {"Cool yellow","#FDEA8D",226},
    {"Earth green","#27462D",141}, {"Camo","#3A7D15",1021}, {"Dark green","#287F47",28},
    {"Bright green","#4B974B",37}, {"Shamrock","#5B9A4C",310}, {"Moss","#7C9C6B",317},
    {"Br. yellowish green","#A4BD47",119}, {"Navy blue","#002060",1011}, {"Deep blue","#2154B9",1012},
    {"Really blue","#0000FF",1010}, {"Bright blue","#0D69AC",23}, {"Steel blue","#527CAE",305},
    {"Medium blue","#6E99CA",102}, {"Light blue","#B4D2E4",45}, {"Bright bluish green","#008F9C",107},
    {"Teal","#12EED4",1018}, {"Pastel blue-green","#9FF3E9",1027}, {"Toothpaste","#00FFFF",1019},
    {"Cyan","#04AFEC",1013}, {"Pastel Blue","#80BBDC",11}, {"Pastel light blue","#AFDDFF",1024},
    {"Bright violet","#6B327C",104}, {"Lavender","#8C5B9F",1023}, {"Lilac","#A75E9B",321},
    {"Magenta","#AA00AA",1015}, {"Royal purple","#6225D1",1031}, {"Alder","#B480FF",1006},
    {"Pastel violet","#B1A7FF",1026}, {"Bright red","#C4281C",21}, {"Really red","#FF0000",1004},
    {"Hot pink","#FF00BF",1032}, {"Pink","#FF66CC",1016}, {"Carnation pink","#FF98DC",330},
    {"Light reddish violet","#E8BAC8",9}, {"Pastel orange","#FFC9C9",1025}, {"Dark taupe","#5A4C42",364},
    {"Cork","#BC9B5D",351}, {"Olive","#C1BE42",1008}, {"Medium green","#A1C48C",29},
    {"Grime","#7F8E64",1022}, {"Sand green","#789082",151}, {"Sand blue","#74869D",135},
    {"Lime green","#00FF00",1020}, {"Pastel green","#CCFFCC",1028}, {"New Yeller","#FFFF00",1009},
    {"Pastel yellow","#FFFFCC",1029}, {"Really black","#111111",1003}, {"Black","#1B2A35",26},
    {"Dark stone grey","#635F62",199}, {"Medium stone grey","#A3A2A5",194}, {"Mid gray","#CDCDCD",1002},
    {"Light stone grey","#E5E4DF",208}, {"White","#F2F3F3",1}, {"Institutional white","#F8F8F8",1001},
};

int BrickColor::NumberForName(const std::string& name) {
    for (const auto& entry : Palette)
        if (name == entry.name)
            return entry.number;
    return -1;
}

int BrickColor::PackedRgbForNumber(int number) {
    for (const auto& entry : Palette) {
        if (entry.number != number)
            continue;
        const char* h = entry.hex;
        if (*h == '#')
            ++h;
        return static_cast<int>(std::strtol(h, nullptr, 16)) & 0xFFFFFF;
    }
    return -1;
}

BrickColor::BrickColor(const std::string &name) : _color(Color3::fromRGB(255, 255, 255))
{
    _name = name;
    _color = Color3(0, 0, 0);

    _r = _color.R;
    _g = _color.G;
    _b = _color.B;
}

// Nearest palette entry to an arbitrary color, by squared RGB distance.
BrickColor::BrickColor(Color3 color) : _color(color)
{
    Color3 rgb = color.toRGB(); // 0-255
    double bestDist = -1;
    const Entry* best = &Palette[0];
    for (const auto& entry : Palette) {
        const char* h = entry.hex;
        if (*h == '#') ++h;
        long packed = std::strtol(h, nullptr, 16);
        double dr = ((packed >> 16) & 0xFF) - rgb.R;
        double dg = ((packed >> 8) & 0xFF) - rgb.G;
        double db = (packed & 0xFF) - rgb.B;
        double dist = dr * dr + dg * dg + db * db;
        if (bestDist < 0 || dist < bestDist) {
            bestDist = dist;
            best = &entry;
        }
    }
    _name = best->name;
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
