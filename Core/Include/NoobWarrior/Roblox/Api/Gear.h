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
// File: Gear.h (Roblox API)
// Started by: Hattozo
// Started on: 11/28/2025
// Description:
#pragma once

namespace NoobWarrior::Roblox {
enum class GearType {
    MeleeWeapons = 0,
    RangedWeapons = 1,
    Explosives = 2,
    PowerUps = 3,
    NavigationEnhancers = 4,
    MusicalInstruments = 5,
    SocialItems = 6,
    BuildingTools = 7,
    Transport = 8
};

enum class GearGenreSetting {
    AllGenres = 0,
    MatchingGenreOnly = 1
};
}