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
// Started on: 8/19/2026
// Description: This file is derived from Roblox-File-Format (https://github.com/MaximumADHD/Roblox-File-Format/blob/main/DataTypes/BrickColor.cs)
#pragma once

#include <cstdint>

namespace NoobWarrior::Roblox::DataTypes {
// Every palette entry Roblox has ever shipped a name for, keyed by its BrickColor number.
// BrickColor.cs writes the enum with sparse explicit values and lets C# fill the runs in
// between, so the numbers here are the ones the engine actually stores, not a dense index.
//
// Names are the C# identifiers verbatim (underscores for spaces, a _2 suffix where Roblox
// reused a display name); the human-readable string lives in the info table, not here.
enum class BrickColorId : int32_t {
    White = 1,
    Grey = 2,
    Light_yellow = 3,
    Brick_yellow = 5,
    Light_green_Mint = 6,
    Light_reddish_violet = 9,
    Pastel_Blue = 11,
    Light_orange_brown = 12,
    Nougat = 18,
    Bright_red = 21,
    Med_reddish_violet = 22,
    Bright_blue = 23,
    Bright_yellow = 24,
    Earth_orange = 25,
    Black = 26,
    Dark_grey = 27,
    Dark_green = 28,
    Medium_green = 29,
    Lig_Yellowich_orange = 36,
    Bright_green = 37,
    Dark_orange = 38,
    Light_bluish_violet = 39,
    Transparent = 40,
    Tr_Red = 41,
    Tr_Lg_blue = 42,
    Tr_Blue = 43,
    Tr_Yellow = 44,
    Light_blue = 45,
    Tr_Flu_Reddish_orange = 47,
    Tr_Green = 48,
    Tr_Flu_Green = 49,
    Phosph_White = 50,
    Light_red = 100,
    Medium_red = 101,
    Medium_blue = 102,
    Light_grey = 103,
    Bright_violet = 104,
    Br_yellowish_orange = 105,
    Bright_orange = 106,
    Bright_bluish_green = 107,
    Earth_yellow = 108,
    Bright_bluish_violet = 110,
    Tr_Brown = 111,
    Medium_bluish_violet = 112,
    Tr_Medi_reddish_violet = 113,
    Med_yellowish_green = 115,
    Med_bluish_green = 116,
    Light_bluish_green = 118,
    Br_yellowish_green = 119,
    Lig_yellowish_green = 120,
    Med_yellowish_orange = 121,
    Br_reddish_orange = 123,
    Bright_reddish_violet = 124,
    Light_orange = 125,
    Tr_Bright_bluish_violet = 126,
    Gold = 127,
    Dark_nougat = 128,
    Silver = 131,
    Neon_orange = 133,
    Neon_green = 134,
    Sand_blue = 135,
    Sand_violet = 136,
    Medium_orange = 137,
    Sand_yellow = 138,
    Earth_blue = 140,
    Earth_green = 141,
    Tr_Flu_Blue = 143,
    Sand_blue_metallic = 145,
    Sand_violet_metallic = 146,
    Sand_yellow_metallic = 147,
    Dark_grey_metallic = 148,
    Black_metallic = 149,
    Light_grey_metallic = 150,
    Sand_green = 151,
    Sand_red = 153,
    Dark_red = 154,
    Tr_Flu_Yellow = 157,
    Tr_Flu_Red = 158,
    Gun_metallic = 168,
    Red_flipflop = 176,
    Yellow_flipflop = 178,
    Silver_flipflop = 179,
    Curry = 180,
    Fire_Yellow = 190,
    Flame_yellowish_orange = 191,
    Reddish_brown = 192,
    Flame_reddish_orange = 193,
    Medium_stone_grey = 194,
    Royal_blue = 195,
    Dark_Royal_blue = 196,
    Bright_reddish_lilac = 198,
    Dark_stone_grey = 199,
    Lemon_metalic = 200,
    Light_stone_grey = 208,
    Dark_Curry = 209,
    Faded_green = 210,
    Turquoise = 211,
    Light_Royal_blue = 212,
    Medium_Royal_blue = 213,
    Rust = 216,
    Brown = 217,
    Reddish_lilac = 218,
    Lilac = 219,
    Light_lilac = 220,
    Bright_purple = 221,
    Light_purple = 222,
    Light_pink = 223,
    Light_brick_yellow = 224,
    Warm_yellowish_orange = 225,
    Cool_yellow = 226,
    Dove_blue = 232,
    Medium_lilac = 268,
    Slime_green = 301,
    Smoky_grey = 302,
    Dark_blue = 303,
    Parsley_green = 304,
    Steel_blue = 305,
    Storm_blue = 306,
    Lapis = 307,
    Dark_indigo = 308,
    Sea_green = 309,
    Shamrock = 310,
    Fossil = 311,
    Mulberry = 312,
    Forest_green = 313,
    Cadet_blue = 314,
    Electric_blue = 315,
    Eggplant = 316,
    Moss = 317,
    Artichoke = 318,
    Sage_green = 319,
    Ghost_grey = 320,
    Lilac_2 = 321,
    Plum = 322,
    Olivine = 323,
    Laurel_green = 324,
    Quill_grey = 325,
    Crimson = 327,
    Mint = 328,
    Baby_blue = 329,
    Carnation_pink = 330,
    Persimmon = 331,
    Maroon = 332,
    Gold_2 = 333,
    Daisy_orange = 334,
    Pearl = 335,
    Fog = 336,
    Salmon = 337,
    Terra_Cotta = 338,
    Cocoa = 339,
    Wheat = 340,
    Buttermilk = 341,
    Mauve = 342,
    Sunrise = 343,
    Tawny = 344,
    Rust_2 = 345,
    Cashmere = 346,
    Khaki = 347,
    Lily_white = 348,
    Seashell = 349,
    Burgundy = 350,
    Cork = 351,
    Burlap = 352,
    Beige = 353,
    Oyster = 354,
    Pine_Cone = 355,
    Fawn_brown = 356,
    Hurricane_grey = 357,
    Cloudy_grey = 358,
    Linen = 359,
    Copper = 360,
    Medium_brown = 361,
    Bronze = 362,
    Flint = 363,
    Dark_taupe = 364,
    Burnt_Sienna = 365,
    Institutional_white = 1001,
    Mid_gray = 1002,
    Really_black = 1003,
    Really_red = 1004,
    Deep_orange = 1005,
    Alder = 1006,
    Dusty_Rose = 1007,
    Olive = 1008,
    New_Yeller = 1009,
    Really_blue = 1010,
    Navy_blue = 1011,
    Deep_blue = 1012,
    Cyan = 1013,
    CGA_brown = 1014,
    Magenta = 1015,
    Pink = 1016,
    Deep_orange_2 = 1017,
    Teal = 1018,
    Toothpaste = 1019,
    Lime_green = 1020,
    Camo = 1021,
    Grime = 1022,
    Lavender = 1023,
    Pastel_light_blue = 1024,
    Pastel_orange = 1025,
    Pastel_violet = 1026,
    Pastel_bluegreen = 1027,
    Pastel_green = 1028,
    Pastel_yellow = 1029,
    Pastel_brown = 1030,
    Royal_purple = 1031,
    Hot_pink = 1032
};

// Serialized as its palette number, which is what the binary format stores. The number is the
// whole value: Chunks/PROP.cpp reads and writes a BrickColor column as a plain int32, so this
// type must stay layout-trivial and must not grow a name or colour field.
//
// Name/colour resolution is a lookup over that number and lives in
// Utility/BrickColors.h (NoobWarrior::Roblox::BrickColors). It is a separate header on purpose:
// the palette table would otherwise be pulled into every one of the 906 generated class headers
// that includes DataTypes.h, and almost none of them need it.
//
// Distinct from NoobWarrior::Roblox::BrickColor (Roblox/DataType/BrickColor.h), the emulator's
// avatar-side type, which carries its own double-based Color3.
struct BrickColor {
    int32_t Number {194}; // Medium stone grey, the engine's default.

    constexpr BrickColor() = default;
    explicit constexpr BrickColor(int32_t number) : Number(number) {}

    // Implicit, mirroring `implicit operator BrickColor(BrickColorId)` (BrickColor.cs:240).
    constexpr BrickColor(BrickColorId id) : Number(static_cast<int32_t>(id)) {}

    // No validity check, matching the C# `implicit operator BrickColorId` (BrickColor.cs:239):
    // a file is free to store a number the palette has no name for, and round-tripping it
    // unchanged matters more than rejecting it. Ask BrickColors::FindById if you need to know.
    constexpr BrickColorId Id() const { return static_cast<BrickColorId>(Number); }

    friend constexpr bool operator==(const BrickColor &, const BrickColor &) = default;
};
}
