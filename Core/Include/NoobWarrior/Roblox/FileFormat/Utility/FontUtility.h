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
// File: FontUtility.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: This file is derived from Roblox-File-Format (https://github.com/MaximumADHD/Roblox-File-Format/blob/main/Utility/FontUtility.cs)
#pragma once
#include <NoobWarrior/Roblox/FileFormat/DataTypes/FontFace.h>

#include <algorithm>
#include <cstdint>
#include <string_view>

namespace NoobWarrior::Roblox::Utility {
using NoobWarrior::Roblox::DataTypes::FontFace;
using NoobWarrior::Roblox::DataTypes::FontStyle;
using NoobWarrior::Roblox::DataTypes::FontWeight;

struct FontFaceEntry {
    int32_t Font;
    std::string_view Family;
    FontWeight Weight;
    FontStyle Style;
};

// Enum.Font.Unknown, which is what the reference returns for a face it does not recognise.
inline constexpr int32_t kUnknownFont = 100;

inline constexpr FontFaceEntry kFontFaces[] = {
    {0, "rbxasset://fonts/families/LegacyArial.json", FontWeight::Regular, FontStyle::Normal},
    {1, "rbxasset://fonts/families/Arial.json", FontWeight::Regular, FontStyle::Normal},
    {2, "rbxasset://fonts/families/Arial.json", FontWeight::Bold, FontStyle::Normal},
    {3, "rbxasset://fonts/families/SourceSansPro.json", FontWeight::Regular, FontStyle::Normal},
    {4, "rbxasset://fonts/families/SourceSansPro.json", FontWeight::Bold, FontStyle::Normal},
    {5, "rbxasset://fonts/families/SourceSansPro.json", FontWeight::Light, FontStyle::Normal},
    {6, "rbxasset://fonts/families/SourceSansPro.json", FontWeight::Regular, FontStyle::Italic},
    {7, "rbxasset://fonts/families/AccanthisADFStd.json", FontWeight::Regular, FontStyle::Normal},
    {8, "rbxasset://fonts/families/Guru.json", FontWeight::Regular, FontStyle::Normal},
    {9, "rbxasset://fonts/families/ComicNeueAngular.json", FontWeight::Regular, FontStyle::Normal},
    {10, "rbxasset://fonts/families/Inconsolata.json", FontWeight::Regular, FontStyle::Normal},
    {11, "rbxasset://fonts/families/HighwayGothic.json", FontWeight::Regular, FontStyle::Normal},
    {12, "rbxasset://fonts/families/Zekton.json", FontWeight::Regular, FontStyle::Normal},
    {13, "rbxasset://fonts/families/PressStart2P.json", FontWeight::Regular, FontStyle::Normal},
    {14, "rbxasset://fonts/families/Balthazar.json", FontWeight::Regular, FontStyle::Normal},
    {15, "rbxasset://fonts/families/RomanAntique.json", FontWeight::Regular, FontStyle::Normal},
    {16, "rbxasset://fonts/families/SourceSansPro.json", FontWeight::SemiBold, FontStyle::Normal},
    {17, "rbxasset://fonts/families/GothamSSm.json", FontWeight::Regular, FontStyle::Normal},
    {18, "rbxasset://fonts/families/GothamSSm.json", FontWeight::Medium, FontStyle::Normal},
    {19, "rbxasset://fonts/families/GothamSSm.json", FontWeight::Bold, FontStyle::Normal},
    {20, "rbxasset://fonts/families/GothamSSm.json", FontWeight::Heavy, FontStyle::Normal},
    {21, "rbxasset://fonts/families/AmaticSC.json", FontWeight::Regular, FontStyle::Normal},
    {22, "rbxasset://fonts/families/Bangers.json", FontWeight::Regular, FontStyle::Normal},
    {23, "rbxasset://fonts/families/Creepster.json", FontWeight::Regular, FontStyle::Normal},
    {24, "rbxasset://fonts/families/DenkOne.json", FontWeight::Regular, FontStyle::Normal},
    {25, "rbxasset://fonts/families/Fondamento.json", FontWeight::Regular, FontStyle::Normal},
    {26, "rbxasset://fonts/families/FredokaOne.json", FontWeight::Regular, FontStyle::Normal},
    {27, "rbxasset://fonts/families/GrenzeGotisch.json", FontWeight::Regular, FontStyle::Normal},
    {28, "rbxasset://fonts/families/IndieFlower.json", FontWeight::Regular, FontStyle::Normal},
    {29, "rbxasset://fonts/families/JosefinSans.json", FontWeight::Regular, FontStyle::Normal},
    {30, "rbxasset://fonts/families/Jura.json", FontWeight::Regular, FontStyle::Normal},
    {31, "rbxasset://fonts/families/Kalam.json", FontWeight::Regular, FontStyle::Normal},
    {32, "rbxasset://fonts/families/LuckiestGuy.json", FontWeight::Regular, FontStyle::Normal},
    {33, "rbxasset://fonts/families/Merriweather.json", FontWeight::Regular, FontStyle::Normal},
    {34, "rbxasset://fonts/families/Michroma.json", FontWeight::Regular, FontStyle::Normal},
    {35, "rbxasset://fonts/families/Nunito.json", FontWeight::Regular, FontStyle::Normal},
    {36, "rbxasset://fonts/families/Oswald.json", FontWeight::Regular, FontStyle::Normal},
    {37, "rbxasset://fonts/families/PatrickHand.json", FontWeight::Regular, FontStyle::Normal},
    {38, "rbxasset://fonts/families/PermanentMarker.json", FontWeight::Regular, FontStyle::Normal},
    {39, "rbxasset://fonts/families/Roboto.json", FontWeight::Regular, FontStyle::Normal},
    {40, "rbxasset://fonts/families/RobotoCondensed.json", FontWeight::Regular, FontStyle::Normal},
    {41, "rbxasset://fonts/families/RobotoMono.json", FontWeight::Regular, FontStyle::Normal},
    {42, "rbxasset://fonts/families/Sarpanch.json", FontWeight::Regular, FontStyle::Normal},
    {43, "rbxasset://fonts/families/SpecialElite.json", FontWeight::Regular, FontStyle::Normal},
    {44, "rbxasset://fonts/families/TitilliumWeb.json", FontWeight::Regular, FontStyle::Normal},
    {45, "rbxasset://fonts/families/Ubuntu.json", FontWeight::Regular, FontStyle::Normal},
    {46, "rbxasset://fonts/families/BuilderSans.json", FontWeight::Regular, FontStyle::Normal},
    {47, "rbxasset://fonts/families/BuilderSans.json", FontWeight::Medium, FontStyle::Normal},
    {48, "rbxasset://fonts/families/BuilderSans.json", FontWeight::Bold, FontStyle::Normal},
    {49, "rbxasset://fonts/families/BuilderSans.json", FontWeight::ExtraBold, FontStyle::Normal},
    {50, "rbxasset://fonts/families/Arimo.json", FontWeight::Regular, FontStyle::Normal},
    {51, "rbxasset://fonts/families/Arimo.json", FontWeight::Bold, FontStyle::Normal},
};

// FontSize is to TextSize what Font is to FontFace. Enum.FontSize.Size8 is 0, so a zero-filled
// FontSize column renders every label at 8px.
struct FontSizeEntry {
    int32_t Pixels;
    int32_t FontSize;
};

// Ascending by pixel size; GetFontSize relies on that ordering.
inline constexpr FontSizeEntry kFontSizes[] = {
    {8, 0}, {9, 1}, {10, 2}, {11, 3}, {12, 4}, {14, 5}, {18, 6}, {24, 7},
    {28, 10}, {32, 11}, {36, 8}, {42, 12}, {48, 9}, {60, 13}, {96, 14},
};

// The FontSize enum value for a pixel size, following FontUtility.cs:326-339: anything past 60 is
// Size96, an exact match wins, otherwise the largest size that still fits.
inline int32_t GetFontSize(int32_t pixels) {
    if (pixels > 60)
        return 14;                                  // Size96
    const FontSizeEntry *best = std::begin(kFontSizes);
    for (const FontSizeEntry &entry : kFontSizes) {
        if (entry.Pixels == pixels)
            return entry.FontSize;
        if (entry.Pixels <= pixels)
            best = &entry;
    }
    // The reference throws below the smallest size, which this port cannot do; the smallest is
    // the only sensible answer anyway.
    return best->FontSize;
}

inline int32_t GetFontSize(float pixels) {
    return GetFontSize(static_cast<int32_t>(pixels));
}

// The pixel size a FontSize enum value stands for, or 0 when it is not one. Named apart from
// GetFontSize because C++ cannot overload on the return type the way the reference does.
inline int32_t GetFontSizePixels(int32_t fontSize) {
    const auto found = std::find_if(std::begin(kFontSizes), std::end(kFontSizes),
        [fontSize](const FontSizeEntry &entry) { return entry.FontSize == fontSize; });
    return found == std::end(kFontSizes) ? 0 : found->Pixels;
}

// The legacy Font enum value for a face, or kUnknownFont when it is not one of them. Only Family,
// Weight and Style take part -- CachedFaceId is a cache key, not part of the font's identity.
inline int32_t GetLegacyFont(const FontFace &face) {
    const auto found = std::find_if(std::begin(kFontFaces), std::end(kFontFaces),
        [&face](const FontFaceEntry &entry) {
            return entry.Family == face.Family && entry.Weight == face.Weight &&
                entry.Style == face.Style;
        });
    return found == std::end(kFontFaces) ? kUnknownFont : found->Font;
}

// False when the value is not a legacy Font, leaving face untouched.
inline bool TryGetFontFace(int32_t font, FontFace &face) {
    const auto found = std::find_if(std::begin(kFontFaces), std::end(kFontFaces),
        [font](const FontFaceEntry &entry) { return entry.Font == font; });
    if (found == std::end(kFontFaces))
        return false;
    face = FontFace(std::string(found->Family), found->Weight, found->Style);
    return true;
}
} // namespace NoobWarrior::Roblox::Utility
