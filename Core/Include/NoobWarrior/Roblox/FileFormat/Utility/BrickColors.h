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
// File: BrickColors.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: This file is derived from Roblox-File-Format (https://github.com/MaximumADHD/Roblox-File-Format/blob/main/Utility/BrickColors.cs)
#pragma once
#include <NoobWarrior/Roblox/FileFormat/DataTypes/BrickColor.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/Color3.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/Color3uint8.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

// Roblox's built-in BrickColor data, transcribed from BrickColors.cs. This is a lookup helper
// over the palette number that DataTypes::BrickColor stores; nothing here changes how a
// BrickColor is serialized.
//
// The tables were generated from the reference C# rather than typed, because a wrong digit in a
// colour or a wrong number on a name is invisible until a place file renders wrong.
namespace NoobWarrior::Roblox::BrickColors {
using DataTypes::BrickColorId;

// BrickColors.cs stores the colour as a packed 0xRRGGBB uint and BrickColor.cs unpacks it into a
// float Color3 in the constructor (BrickColor.cs:250-259). Keeping only the byte form and
// deriving the float one means the two can never disagree.
struct BrickColorInfo {
    BrickColorId Id {};
    std::string_view Name;
    DataTypes::Color3uint8 Color8 {};

    constexpr int32_t Number() const { return static_cast<int32_t>(Id); }
    constexpr DataTypes::Color3 Color() const { return Color8.ToColor3(); }
    constexpr DataTypes::BrickColor ToBrickColor() const { return DataTypes::BrickColor(Id); }

    constexpr float R() const { return Color8.R / 255.0f; }
    constexpr float G() const { return Color8.G / 255.0f; }
    constexpr float B() const { return Color8.B / 255.0f; }
};

// In BrickColors.cs InfoMap order, which is ascending by number. Two lookups below rely on that:
// FindByNumber binary-searches it, and FindByName / FromRGB resolve ties by taking the first
// match, which is how the C# Dictionary enumeration order resolves the four reused display names
// (Gold, Rust, Lilac, Deep orange) and the FromRGB `dist < closest` comparison.
inline constexpr std::array<BrickColorInfo, 208> kInfoMap { {
    {BrickColorId::White,                   "White", {242, 243, 243}},
    {BrickColorId::Grey,                    "Grey", {161, 165, 162}},
    {BrickColorId::Light_yellow,            "Light yellow", {249, 233, 153}},
    {BrickColorId::Brick_yellow,            "Brick yellow", {215, 197, 154}},
    {BrickColorId::Light_green_Mint,        "Light green (Mint)", {194, 218, 184}},
    {BrickColorId::Light_reddish_violet,    "Light reddish violet", {232, 186, 200}},
    {BrickColorId::Pastel_Blue,             "Pastel Blue", {128, 187, 219}},
    {BrickColorId::Light_orange_brown,      "Light orange brown", {203, 132, 66}},
    {BrickColorId::Nougat,                  "Nougat", {204, 142, 105}},
    {BrickColorId::Bright_red,              "Bright red", {196, 40, 28}},
    {BrickColorId::Med_reddish_violet,      "Med. reddish violet", {196, 112, 160}},
    {BrickColorId::Bright_blue,             "Bright blue", {13, 105, 172}},
    {BrickColorId::Bright_yellow,           "Bright yellow", {245, 205, 48}},
    {BrickColorId::Earth_orange,            "Earth orange", {98, 71, 50}},
    {BrickColorId::Black,                   "Black", {27, 42, 53}},
    {BrickColorId::Dark_grey,               "Dark grey", {109, 110, 108}},
    {BrickColorId::Dark_green,              "Dark green", {40, 127, 71}},
    {BrickColorId::Medium_green,            "Medium green", {161, 196, 140}},
    {BrickColorId::Lig_Yellowich_orange,    "Lig. Yellowich orange", {243, 207, 155}},
    {BrickColorId::Bright_green,            "Bright green", {75, 151, 75}},
    {BrickColorId::Dark_orange,             "Dark orange", {160, 95, 53}},
    {BrickColorId::Light_bluish_violet,     "Light bluish violet", {193, 202, 222}},
    {BrickColorId::Transparent,             "Transparent", {236, 236, 236}},
    {BrickColorId::Tr_Red,                  "Tr. Red", {205, 84, 75}},
    {BrickColorId::Tr_Lg_blue,              "Tr. Lg blue", {193, 223, 240}},
    {BrickColorId::Tr_Blue,                 "Tr. Blue", {123, 182, 232}},
    {BrickColorId::Tr_Yellow,               "Tr. Yellow", {247, 241, 141}},
    {BrickColorId::Light_blue,              "Light blue", {180, 210, 228}},
    {BrickColorId::Tr_Flu_Reddish_orange,   "Tr. Flu. Reddish orange", {217, 133, 108}},
    {BrickColorId::Tr_Green,                "Tr. Green", {132, 182, 141}},
    {BrickColorId::Tr_Flu_Green,            "Tr. Flu. Green", {248, 241, 132}},
    {BrickColorId::Phosph_White,            "Phosph. White", {236, 232, 222}},
    {BrickColorId::Light_red,               "Light red", {238, 196, 182}},
    {BrickColorId::Medium_red,              "Medium red", {218, 134, 122}},
    {BrickColorId::Medium_blue,             "Medium blue", {110, 153, 202}},
    {BrickColorId::Light_grey,              "Light grey", {199, 193, 183}},
    {BrickColorId::Bright_violet,           "Bright violet", {107, 50, 124}},
    {BrickColorId::Br_yellowish_orange,     "Br. yellowish orange", {226, 155, 64}},
    {BrickColorId::Bright_orange,           "Bright orange", {218, 133, 65}},
    {BrickColorId::Bright_bluish_green,     "Bright bluish green", {0, 143, 156}},
    {BrickColorId::Earth_yellow,            "Earth yellow", {104, 92, 67}},
    {BrickColorId::Bright_bluish_violet,    "Bright bluish violet", {67, 84, 147}},
    {BrickColorId::Tr_Brown,                "Tr. Brown", {191, 183, 177}},
    {BrickColorId::Medium_bluish_violet,    "Medium bluish violet", {104, 116, 172}},
    {BrickColorId::Tr_Medi_reddish_violet,  "Tr. Medi. reddish violet", {229, 173, 200}},
    {BrickColorId::Med_yellowish_green,     "Med. yellowish green", {199, 210, 60}},
    {BrickColorId::Med_bluish_green,        "Med. bluish green", {85, 165, 175}},
    {BrickColorId::Light_bluish_green,      "Light bluish green", {183, 215, 213}},
    {BrickColorId::Br_yellowish_green,      "Br. yellowish green", {164, 189, 71}},
    {BrickColorId::Lig_yellowish_green,     "Lig. yellowish green", {217, 228, 167}},
    {BrickColorId::Med_yellowish_orange,    "Med. yellowish orange", {231, 172, 88}},
    {BrickColorId::Br_reddish_orange,       "Br. reddish orange", {211, 111, 76}},
    {BrickColorId::Bright_reddish_violet,   "Bright reddish violet", {146, 57, 120}},
    {BrickColorId::Light_orange,            "Light orange", {234, 184, 146}},
    {BrickColorId::Tr_Bright_bluish_violet, "Tr. Bright bluish violet", {165, 165, 203}},
    {BrickColorId::Gold,                    "Gold", {220, 188, 129}},
    {BrickColorId::Dark_nougat,             "Dark nougat", {174, 122, 89}},
    {BrickColorId::Silver,                  "Silver", {156, 163, 168}},
    {BrickColorId::Neon_orange,             "Neon orange", {213, 115, 61}},
    {BrickColorId::Neon_green,              "Neon green", {216, 221, 86}},
    {BrickColorId::Sand_blue,               "Sand blue", {116, 134, 157}},
    {BrickColorId::Sand_violet,             "Sand violet", {135, 124, 144}},
    {BrickColorId::Medium_orange,           "Medium orange", {224, 152, 100}},
    {BrickColorId::Sand_yellow,             "Sand yellow", {149, 138, 115}},
    {BrickColorId::Earth_blue,              "Earth blue", {32, 58, 86}},
    {BrickColorId::Earth_green,             "Earth green", {39, 70, 45}},
    {BrickColorId::Tr_Flu_Blue,             "Tr. Flu. Blue", {207, 226, 247}},
    {BrickColorId::Sand_blue_metallic,      "Sand blue metallic", {121, 136, 161}},
    {BrickColorId::Sand_violet_metallic,    "Sand violet metallic", {149, 142, 163}},
    {BrickColorId::Sand_yellow_metallic,    "Sand yellow metallic", {147, 135, 103}},
    {BrickColorId::Dark_grey_metallic,      "Dark grey metallic", {87, 88, 87}},
    {BrickColorId::Black_metallic,          "Black metallic", {22, 29, 50}},
    {BrickColorId::Light_grey_metallic,     "Light grey metallic", {171, 173, 172}},
    {BrickColorId::Sand_green,              "Sand green", {120, 144, 130}},
    {BrickColorId::Sand_red,                "Sand red", {149, 121, 119}},
    {BrickColorId::Dark_red,                "Dark red", {123, 46, 47}},
    {BrickColorId::Tr_Flu_Yellow,           "Tr. Flu. Yellow", {255, 246, 123}},
    {BrickColorId::Tr_Flu_Red,              "Tr. Flu. Red", {225, 164, 194}},
    {BrickColorId::Gun_metallic,            "Gun metallic", {117, 108, 98}},
    {BrickColorId::Red_flipflop,            "Red flip/flop", {151, 105, 91}},
    {BrickColorId::Yellow_flipflop,         "Yellow flip/flop", {180, 132, 85}},
    {BrickColorId::Silver_flipflop,         "Silver flip/flop", {137, 135, 136}},
    {BrickColorId::Curry,                   "Curry", {215, 169, 75}},
    {BrickColorId::Fire_Yellow,             "Fire Yellow", {249, 214, 46}},
    {BrickColorId::Flame_yellowish_orange,  "Flame yellowish orange", {232, 171, 45}},
    {BrickColorId::Reddish_brown,           "Reddish brown", {105, 64, 40}},
    {BrickColorId::Flame_reddish_orange,    "Flame reddish orange", {207, 96, 36}},
    {BrickColorId::Medium_stone_grey,       "Medium stone grey", {163, 162, 165}},
    {BrickColorId::Royal_blue,              "Royal blue", {70, 103, 164}},
    {BrickColorId::Dark_Royal_blue,         "Dark Royal blue", {35, 71, 139}},
    {BrickColorId::Bright_reddish_lilac,    "Bright reddish lilac", {142, 66, 133}},
    {BrickColorId::Dark_stone_grey,         "Dark stone grey", {99, 95, 98}},
    {BrickColorId::Lemon_metalic,           "Lemon metalic", {130, 138, 93}},
    {BrickColorId::Light_stone_grey,        "Light stone grey", {229, 228, 223}},
    {BrickColorId::Dark_Curry,              "Dark Curry", {176, 142, 68}},
    {BrickColorId::Faded_green,             "Faded green", {112, 149, 120}},
    {BrickColorId::Turquoise,               "Turquoise", {121, 181, 181}},
    {BrickColorId::Light_Royal_blue,        "Light Royal blue", {159, 195, 233}},
    {BrickColorId::Medium_Royal_blue,       "Medium Royal blue", {108, 129, 183}},
    {BrickColorId::Rust,                    "Rust", {144, 76, 42}},
    {BrickColorId::Brown,                   "Brown", {124, 92, 70}},
    {BrickColorId::Reddish_lilac,           "Reddish lilac", {150, 112, 159}},
    {BrickColorId::Lilac,                   "Lilac", {107, 98, 155}},
    {BrickColorId::Light_lilac,             "Light lilac", {167, 169, 206}},
    {BrickColorId::Bright_purple,           "Bright purple", {205, 98, 152}},
    {BrickColorId::Light_purple,            "Light purple", {228, 173, 200}},
    {BrickColorId::Light_pink,              "Light pink", {220, 144, 149}},
    {BrickColorId::Light_brick_yellow,      "Light brick yellow", {240, 213, 160}},
    {BrickColorId::Warm_yellowish_orange,   "Warm yellowish orange", {235, 184, 127}},
    {BrickColorId::Cool_yellow,             "Cool yellow", {253, 234, 141}},
    {BrickColorId::Dove_blue,               "Dove blue", {125, 187, 221}},
    {BrickColorId::Medium_lilac,            "Medium lilac", {52, 43, 117}},
    {BrickColorId::Slime_green,             "Slime green", {80, 109, 84}},
    {BrickColorId::Smoky_grey,              "Smoky grey", {91, 93, 105}},
    {BrickColorId::Dark_blue,               "Dark blue", {0, 16, 176}},
    {BrickColorId::Parsley_green,           "Parsley green", {44, 101, 29}},
    {BrickColorId::Steel_blue,              "Steel blue", {82, 124, 174}},
    {BrickColorId::Storm_blue,              "Storm blue", {51, 88, 130}},
    {BrickColorId::Lapis,                   "Lapis", {16, 42, 220}},
    {BrickColorId::Dark_indigo,             "Dark indigo", {61, 21, 133}},
    {BrickColorId::Sea_green,               "Sea green", {52, 142, 64}},
    {BrickColorId::Shamrock,                "Shamrock", {91, 154, 76}},
    {BrickColorId::Fossil,                  "Fossil", {159, 161, 172}},
    {BrickColorId::Mulberry,                "Mulberry", {89, 34, 89}},
    {BrickColorId::Forest_green,            "Forest green", {31, 128, 29}},
    {BrickColorId::Cadet_blue,              "Cadet blue", {159, 173, 192}},
    {BrickColorId::Electric_blue,           "Electric blue", {9, 137, 207}},
    {BrickColorId::Eggplant,                "Eggplant", {123, 0, 123}},
    {BrickColorId::Moss,                    "Moss", {124, 156, 107}},
    {BrickColorId::Artichoke,               "Artichoke", {138, 171, 133}},
    {BrickColorId::Sage_green,              "Sage green", {185, 196, 177}},
    {BrickColorId::Ghost_grey,              "Ghost grey", {202, 203, 209}},
    {BrickColorId::Lilac_2,                 "Lilac", {167, 94, 155}},
    {BrickColorId::Plum,                    "Plum", {123, 47, 123}},
    {BrickColorId::Olivine,                 "Olivine", {148, 190, 129}},
    {BrickColorId::Laurel_green,            "Laurel green", {168, 189, 153}},
    {BrickColorId::Quill_grey,              "Quill grey", {223, 223, 222}},
    {BrickColorId::Crimson,                 "Crimson", {151, 0, 0}},
    {BrickColorId::Mint,                    "Mint", {177, 229, 166}},
    {BrickColorId::Baby_blue,               "Baby blue", {152, 194, 219}},
    {BrickColorId::Carnation_pink,          "Carnation pink", {255, 152, 220}},
    {BrickColorId::Persimmon,               "Persimmon", {255, 89, 89}},
    {BrickColorId::Maroon,                  "Maroon", {117, 0, 0}},
    {BrickColorId::Gold_2,                  "Gold", {239, 184, 56}},
    {BrickColorId::Daisy_orange,            "Daisy orange", {248, 217, 109}},
    {BrickColorId::Pearl,                   "Pearl", {231, 231, 236}},
    {BrickColorId::Fog,                     "Fog", {199, 212, 228}},
    {BrickColorId::Salmon,                  "Salmon", {255, 148, 148}},
    {BrickColorId::Terra_Cotta,             "Terra Cotta", {190, 104, 98}},
    {BrickColorId::Cocoa,                   "Cocoa", {86, 36, 36}},
    {BrickColorId::Wheat,                   "Wheat", {241, 231, 199}},
    {BrickColorId::Buttermilk,              "Buttermilk", {254, 243, 187}},
    {BrickColorId::Mauve,                   "Mauve", {224, 178, 208}},
    {BrickColorId::Sunrise,                 "Sunrise", {212, 144, 189}},
    {BrickColorId::Tawny,                   "Tawny", {150, 85, 85}},
    {BrickColorId::Rust_2,                  "Rust", {143, 76, 42}},
    {BrickColorId::Cashmere,                "Cashmere", {211, 190, 150}},
    {BrickColorId::Khaki,                   "Khaki", {226, 220, 188}},
    {BrickColorId::Lily_white,              "Lily white", {237, 234, 234}},
    {BrickColorId::Seashell,                "Seashell", {233, 218, 218}},
    {BrickColorId::Burgundy,                "Burgundy", {136, 62, 62}},
    {BrickColorId::Cork,                    "Cork", {188, 155, 93}},
    {BrickColorId::Burlap,                  "Burlap", {199, 172, 120}},
    {BrickColorId::Beige,                   "Beige", {202, 191, 163}},
    {BrickColorId::Oyster,                  "Oyster", {187, 179, 178}},
    {BrickColorId::Pine_Cone,               "Pine Cone", {108, 88, 75}},
    {BrickColorId::Fawn_brown,              "Fawn brown", {160, 132, 79}},
    {BrickColorId::Hurricane_grey,          "Hurricane grey", {149, 137, 136}},
    {BrickColorId::Cloudy_grey,             "Cloudy grey", {171, 168, 158}},
    {BrickColorId::Linen,                   "Linen", {175, 148, 131}},
    {BrickColorId::Copper,                  "Copper", {150, 103, 102}},
    {BrickColorId::Medium_brown,            "Medium brown", {86, 66, 54}},
    {BrickColorId::Bronze,                  "Bronze", {126, 104, 63}},
    {BrickColorId::Flint,                   "Flint", {105, 102, 92}},
    {BrickColorId::Dark_taupe,              "Dark taupe", {90, 76, 66}},
    {BrickColorId::Burnt_Sienna,            "Burnt Sienna", {106, 57, 9}},
    {BrickColorId::Institutional_white,     "Institutional white", {248, 248, 248}},
    {BrickColorId::Mid_gray,                "Mid gray", {205, 205, 205}},
    {BrickColorId::Really_black,            "Really black", {17, 17, 17}},
    {BrickColorId::Really_red,              "Really red", {255, 0, 0}},
    {BrickColorId::Deep_orange,             "Deep orange", {255, 176, 0}},
    {BrickColorId::Alder,                   "Alder", {180, 128, 255}},
    {BrickColorId::Dusty_Rose,              "Dusty Rose", {163, 75, 75}},
    {BrickColorId::Olive,                   "Olive", {193, 190, 66}},
    {BrickColorId::New_Yeller,              "New Yeller", {255, 255, 0}},
    {BrickColorId::Really_blue,             "Really blue", {0, 0, 255}},
    {BrickColorId::Navy_blue,               "Navy blue", {0, 32, 96}},
    {BrickColorId::Deep_blue,               "Deep blue", {33, 84, 185}},
    {BrickColorId::Cyan,                    "Cyan", {4, 175, 236}},
    {BrickColorId::CGA_brown,               "CGA brown", {170, 85, 0}},
    {BrickColorId::Magenta,                 "Magenta", {170, 0, 170}},
    {BrickColorId::Pink,                    "Pink", {255, 102, 204}},
    {BrickColorId::Deep_orange_2,           "Deep orange", {255, 175, 0}},
    {BrickColorId::Teal,                    "Teal", {18, 238, 212}},
    {BrickColorId::Toothpaste,              "Toothpaste", {0, 255, 255}},
    {BrickColorId::Lime_green,              "Lime green", {0, 255, 0}},
    {BrickColorId::Camo,                    "Camo", {58, 125, 21}},
    {BrickColorId::Grime,                   "Grime", {127, 142, 100}},
    {BrickColorId::Lavender,                "Lavender", {140, 91, 159}},
    {BrickColorId::Pastel_light_blue,       "Pastel light blue", {175, 221, 255}},
    {BrickColorId::Pastel_orange,           "Pastel orange", {255, 201, 201}},
    {BrickColorId::Pastel_violet,           "Pastel violet", {177, 167, 255}},
    {BrickColorId::Pastel_bluegreen,        "Pastel blue-green", {159, 243, 233}},
    {BrickColorId::Pastel_green,            "Pastel green", {204, 255, 204}},
    {BrickColorId::Pastel_yellow,           "Pastel yellow", {255, 255, 204}},
    {BrickColorId::Pastel_brown,            "Pastel brown", {255, 204, 153}},
    {BrickColorId::Royal_purple,            "Royal purple", {98, 37, 209}},
    {BrickColorId::Hot_pink,                "Hot pink", {255, 0, 191}},
} };

// The 128 colours the Studio colour picker offers, in picker order (BrickColors.cs:25-155).
// Unrelated to the numbers: this is a display ordering, and Palette(index) indexes it.
inline constexpr std::array<BrickColorId, 128> kPalette { {
    BrickColorId::Earth_green, BrickColorId::Slime_green, BrickColorId::Bright_bluish_green,
    BrickColorId::Black, BrickColorId::Deep_blue, BrickColorId::Dark_blue,
    BrickColorId::Navy_blue, BrickColorId::Parsley_green, BrickColorId::Dark_green,
    BrickColorId::Teal, BrickColorId::Smoky_grey, BrickColorId::Steel_blue,
    BrickColorId::Storm_blue, BrickColorId::Lapis, BrickColorId::Dark_indigo,
    BrickColorId::Camo, BrickColorId::Sea_green, BrickColorId::Shamrock,
    BrickColorId::Toothpaste, BrickColorId::Sand_blue, BrickColorId::Medium_blue,
    BrickColorId::Bright_blue, BrickColorId::Really_blue, BrickColorId::Mulberry,
    BrickColorId::Forest_green, BrickColorId::Bright_green, BrickColorId::Grime,
    BrickColorId::Lime_green, BrickColorId::Pastel_bluegreen, BrickColorId::Fossil,
    BrickColorId::Electric_blue, BrickColorId::Lavender, BrickColorId::Royal_purple,
    BrickColorId::Eggplant, BrickColorId::Sand_green, BrickColorId::Moss,
    BrickColorId::Artichoke, BrickColorId::Sage_green, BrickColorId::Pastel_light_blue,
    BrickColorId::Cadet_blue, BrickColorId::Cyan, BrickColorId::Alder,
    BrickColorId::Lilac, BrickColorId::Plum, BrickColorId::Bright_violet,
    BrickColorId::Olive, BrickColorId::Br_yellowish_green, BrickColorId::Olivine,
    BrickColorId::Laurel_green, BrickColorId::Quill_grey, BrickColorId::Ghost_grey,
    BrickColorId::Pastel_Blue, BrickColorId::Pastel_violet, BrickColorId::Pink,
    BrickColorId::Hot_pink, BrickColorId::Magenta, BrickColorId::Crimson,
    BrickColorId::Deep_orange, BrickColorId::New_Yeller, BrickColorId::Medium_green,
    BrickColorId::Mint, BrickColorId::Pastel_green, BrickColorId::Light_stone_grey,
    BrickColorId::Light_blue, BrickColorId::Baby_blue, BrickColorId::Carnation_pink,
    BrickColorId::Persimmon, BrickColorId::Really_red, BrickColorId::Bright_red,
    BrickColorId::Maroon, BrickColorId::Gold, BrickColorId::Bright_yellow,
    BrickColorId::Daisy_orange, BrickColorId::Cool_yellow, BrickColorId::Pastel_yellow,
    BrickColorId::Pearl, BrickColorId::Fog, BrickColorId::Mauve,
    BrickColorId::Sunrise, BrickColorId::Terra_Cotta, BrickColorId::Dusty_Rose,
    BrickColorId::Cocoa, BrickColorId::Neon_orange, BrickColorId::Bright_orange,
    BrickColorId::Wheat, BrickColorId::Buttermilk, BrickColorId::Institutional_white,
    BrickColorId::White, BrickColorId::Light_reddish_violet, BrickColorId::Pastel_orange,
    BrickColorId::Salmon, BrickColorId::Tawny, BrickColorId::Rust,
    BrickColorId::CGA_brown, BrickColorId::Br_yellowish_orange, BrickColorId::Cashmere,
    BrickColorId::Khaki, BrickColorId::Lily_white, BrickColorId::Seashell,
    BrickColorId::Pastel_brown, BrickColorId::Light_orange, BrickColorId::Medium_red,
    BrickColorId::Burgundy, BrickColorId::Reddish_brown, BrickColorId::Cork,
    BrickColorId::Burlap, BrickColorId::Beige, BrickColorId::Oyster,
    BrickColorId::Mid_gray, BrickColorId::Brick_yellow, BrickColorId::Nougat,
    BrickColorId::Brown, BrickColorId::Pine_Cone, BrickColorId::Fawn_brown,
    BrickColorId::Sand_red, BrickColorId::Hurricane_grey, BrickColorId::Cloudy_grey,
    BrickColorId::Linen, BrickColorId::Copper, BrickColorId::Dark_orange,
    BrickColorId::Medium_brown, BrickColorId::Bronze, BrickColorId::Dark_stone_grey,
    BrickColorId::Medium_stone_grey, BrickColorId::Flint, BrickColorId::Dark_taupe,
    BrickColorId::Burnt_Sienna, BrickColorId::Really_black,
} };

inline constexpr BrickColorId kDefaultId = BrickColorId::Medium_stone_grey;

constexpr const BrickColorInfo *FindById(BrickColorId id) {
    // kInfoMap is sorted by number, so this can bisect; a BrickColor column carries one value per
    // instance of its class, and a place has a lot of parts.
    std::size_t lo = 0, hi = kInfoMap.size();
    const int32_t want = static_cast<int32_t>(id);

    while (lo < hi) {
        const std::size_t mid = lo + (hi - lo) / 2;
        const int32_t got = kInfoMap[mid].Number();

        if (got == want)
            return &kInfoMap[mid];
        else if (got < want)
            lo = mid + 1;
        else
            hi = mid;
    }

    return nullptr;
}

// nullptr for a number the palette has no entry for. The C# equivalent is the Enum.IsDefined
// test inside FromNumber (BrickColor.cs:301); it is exposed separately here so a caller can tell
// "unknown number" apart from "deliberately Medium stone grey".
constexpr const BrickColorInfo *FindByNumber(int32_t number) {
    return FindById(static_cast<BrickColorId>(number));
}

// First entry with this display name, or nullptr. C#'s ByName is built by skipping later
// duplicates (BrickColor.cs:267-275), so "Gold" resolves to 127 and never 333.
constexpr const BrickColorInfo *FindByName(std::string_view name) {
    for (const BrickColorInfo &info : kInfoMap)
        if (info.Name == name)
            return &info;

    return nullptr;
}

// BrickColor.cs's DefaultId (BrickColor.cs:237). Every fallible lookup that has to return
// something falls back to this.
constexpr const BrickColorInfo &Default() {
    // Medium stone grey is in the table, so this never null-derefs.
    return *FindById(kDefaultId);
}

// FromNumber (BrickColor.cs:297-305): an unknown number silently becomes the default rather than
// failing, because that is what Roblox does when it loads a file with a stale palette number.
constexpr const BrickColorInfo &FromNumber(int32_t number) {
    const BrickColorInfo *info = FindByNumber(number);
    return info != nullptr ? *info : Default();
}

constexpr const BrickColorInfo &FromBrickColor(DataTypes::BrickColor color) {
    return FromNumber(color.Number);
}

// FromRGB (BrickColor.cs:312-335): nearest palette entry by summed per-channel distance, with an
// early out on an exact match. Not a perceptual metric -- it is reproduced as-is so a colour
// converted here lands on the same BrickColor the reference would pick.
constexpr const BrickColorInfo &FromRGB(float r = 0, float g = 0, float b = 0) {
    const BrickColorInfo *bestMatch = &Default();
    float closest = 3.402823466e+38f; // float.MaxValue

    for (const BrickColorInfo &info : kInfoMap) {
        const float dr = info.R() - r, dg = info.G() - g, db = info.B() - b;
        const float dist = (dr < 0 ? -dr : dr) + (dg < 0 ? -dg : dg) + (db < 0 ? -db : db);

        if (dist < closest) {
            if (dist == 0.0f)
                return info;

            bestMatch = &info;
            closest = dist;
        }
    }

    return *bestMatch;
}

constexpr const BrickColorInfo &FromColor3(DataTypes::Color3 color) {
    return FromRGB(color.R, color.G, color.B);
}

constexpr const BrickColorInfo &FromColor3uint8(DataTypes::Color3uint8 color) {
    return FromColor3(color.ToColor3());
}

// Palette(index) (BrickColor.cs:343-349). The reference throws on an out-of-range index; this
// returns nullptr instead, per the house rule that fallible operations do not throw.
constexpr const BrickColorInfo *Palette(std::size_t index) {
    if (index >= kPalette.size())
        return nullptr;

    return FindById(kPalette[index]);
}

// The named shorthands at the bottom of BrickColor.cs (BrickColor.cs:351-358). Random() is
// deliberately not ported: it needs an RNG (BrickColor.cs:337-341) and nothing in the file
// format has any reason to pick a colour at random.
constexpr const BrickColorInfo &Red() { return *FindById(BrickColorId::Bright_red); }
constexpr const BrickColorInfo &Gray() { return *FindById(BrickColorId::Medium_stone_grey); }
constexpr const BrickColorInfo &Blue() { return *FindById(BrickColorId::Bright_blue); }
constexpr const BrickColorInfo &Black() { return *FindById(BrickColorId::Black); }
constexpr const BrickColorInfo &Green() { return *FindById(BrickColorId::Bright_green); }
constexpr const BrickColorInfo &White() { return *FindById(BrickColorId::White); }
constexpr const BrickColorInfo &Yellow() { return *FindById(BrickColorId::Bright_yellow); }
constexpr const BrickColorInfo &DarkGray() { return *FindById(BrickColorId::Dark_stone_grey); }
}
