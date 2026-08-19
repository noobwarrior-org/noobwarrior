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
// File: LostEnumValue.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: This file is derived from Roblox-File-Format (https://github.com/MaximumADHD/Roblox-File-Format/blob/main/Utility/LostEnumValue.cs)
#pragma once
#include <cstdint>
#include <array>
#include <string_view>

// A lost enum value is one Roblox deleted outright, leaving a hole in the numbering that no
// later member ever filled. LostEnumValue.cs is the C# attribute that records those holes; the
// data itself lives in the generator, Update/Plugin/GenerateApiDump/LostEnumValues.luau, and is
// stamped onto a placeholder member whose own value is the survivor the removed name folded
// into -- "[LostEnumValue(MapTo = 7)] _Laser = Script" means raw 7 used to be BinType.Laser and
// now reads as BinType.Script.
//
// C# gets away with never consuming the attribute, because a C# enum holds any int: casting 7
// to BinType just yields an unnamed 7. This port's Generated/Enums.h drops the placeholders
// entirely (BinType is Script/GameTool/Grab/Clone/Hammer and nothing else), so the hole is not
// representable and the mapping has to be applied rather than merely documented.
//
// It matters here for the reason none of it matters upstream: noobWarrior reads a place written
// by an engine older than the 0.735 dump the registry came from. A 2008-era place really does
// carry BinType 7 on a HopperBin and InputType 4 on a Part's *SurfaceInput -- values the dump no
// longer lists and none of 0.463/0.574/0.729 still understand either.
//
// This is a lookup, not a policy. Rewriting a value is lossy, so the decision to rewrite belongs
// to the reader that knows whether it is loading a file or round-tripping one.
namespace NoobWarrior::Roblox::Utility {
struct LostEnumValue {
    // Enums.h enum this hole belongs to, e.g. "BinType".
    std::string_view EnumName;
    // The name the value carried before Roblox removed it.
    std::string_view LostName;
    // The raw value a file can still carry. Named after the reference's attribute property.
    int32_t MapTo;
    // The value that survives in the dump, taken from the member the placeholder was assigned
    // to in Generated/Enums.cs. Every one of them is the enum's zero member, because Roblox
    // retired these by folding the whole feature back onto "none".
    int32_t Replacement;
    // The surviving member's name, so a log line can say what it became.
    std::string_view ReplacementName;
};

// Transcribed from Update/Plugin/GenerateApiDump/LostEnumValues.luau; the replacements are the
// members Generated/Enums.cs assigns each placeholder to.
inline constexpr std::array<LostEnumValue, 14> kLostEnumValues {{
    {"BinType", "Slingshot", 5, 0, "Script"},
    {"BinType", "Rocket", 6, 0, "Script"},
    {"BinType", "Laser", 7, 0, "Script"},

    {"InputType", "LeftTread", 1, 0, "NoInput"},
    {"InputType", "RightTread", 2, 0, "NoInput"},
    {"InputType", "Steer", 3, 0, "NoInput"},
    {"InputType", "Throttle", 4, 0, "NoInput"},
    {"InputType", "UpDown", 6, 0, "NoInput"},
    {"InputType", "Action1", 7, 0, "NoInput"},
    {"InputType", "Action2", 8, 0, "NoInput"},
    {"InputType", "Action3", 9, 0, "NoInput"},
    {"InputType", "Action4", 10, 0, "NoInput"},
    {"InputType", "Action5", 11, 0, "NoInput"},

    {"SurfaceType", "Unjoinable", 9, 0, "Smooth"},
}};

// Null when the value is still a live member, or when the enum has no holes at all -- which is
// every enum but these three.
constexpr const LostEnumValue *FindLostEnumValue(std::string_view enumName, uint32_t value) {
    for (const LostEnumValue &lost : kLostEnumValues) {
        if (lost.EnumName == enumName && static_cast<uint32_t>(lost.MapTo) == value)
            return &lost;
    }
    return nullptr;
}

// The value to store for a raw enum read out of a file: the replacement when the file carries a
// hole, the value itself otherwise. Identity for every enum that has no holes, so it is safe to
// run over every enum property rather than special-casing the three.
constexpr uint32_t ResolveLostEnumValue(std::string_view enumName, uint32_t value) {
    if (const LostEnumValue *lost = FindLostEnumValue(enumName, value))
        return static_cast<uint32_t>(lost->Replacement);
    return value;
}
} // namespace NoobWarrior::Roblox::Utility
