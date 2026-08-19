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
// File: FontFace.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Ported from RobloxFiles.DataTypes.FontFace (MaximumADHD/Roblox-File-Format).
#pragma once

#include <cstdint>
#include <string>

namespace NoobWarrior::Roblox::DataTypes {
enum class FontWeight : uint16_t {
    Thin = 100,
    ExtraLight = 200,
    Light = 300,
    Regular = 400,
    Medium = 500,
    SemiBold = 600,
    Bold = 700,
    ExtraBold = 800,
    Heavy = 900,
};

enum class FontStyle : uint8_t {
    Normal = 0,
    Italic = 1,
};

struct FontFace {
    // FontFace.cs:12 defaults the family to LegacyArial, not to an empty string -- an empty family
    // is not a font the engine can resolve, so a default-constructed value must name a real one.
    std::string Family {"rbxasset://fonts/families/LegacyArial.json"};
    FontWeight Weight {FontWeight::Regular};
    FontStyle Style {FontStyle::Normal};
    std::string CachedFaceId;

    FontFace() = default;
    FontFace(std::string family, FontWeight weight, FontStyle style,
             std::string cachedFaceId = {}) :
        Family(std::move(family)), Weight(weight), Style(style),
        CachedFaceId(std::move(cachedFaceId)) {}

    friend bool operator==(const FontFace &, const FontFace &) = default;
};
}
