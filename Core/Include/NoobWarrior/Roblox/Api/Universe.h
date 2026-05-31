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
// File: Universe.h
// Started by: Hattozo
// Started on: 3/15/2025
// Description: C-struct/enum representations of universe (game) data from the Roblox API.
#pragma once

namespace NoobWarrior::Roblox {
    constexpr int GenreCount = 14;
    enum class Genre {
        All = 0, TownAndCity = 1, Fantasy = 2, SciFi = 3, Ninja = 4,
        Scary = 5, Pirate = 6, Adventure = 7, Sports = 8, Funny = 9,
        WildWest = 10, War = 11, SkatePark = 12, Tutorial = 13
    };

    inline const char* GenreAsTranslatableString(Genre genre) {
        switch (genre) {
        case Genre::TownAndCity: return "Town and City";
        case Genre::Fantasy: return "Fantasy";
        case Genre::SciFi: return "Sci-Fi";
        case Genre::Ninja: return "Ninja";
        case Genre::Scary: return "Scary";
        case Genre::Pirate: return "Pirate";
        case Genre::Adventure: return "Adventure";
        case Genre::Sports: return "Sports";
        case Genre::Funny: return "Funny";
        case Genre::WildWest: return "Wild West";
        case Genre::War: return "War";
        case Genre::SkatePark: return "Skate Park";
        case Genre::Tutorial: return "Tutorial";
        default: return "All";
        }
    }

    enum class UniverseAvatarType { MorphToR6 = 1, PlayerChoice = 2, MorphToR15 = 3 };

    inline const char* UniverseAvatarTypeAsTranslatableString(UniverseAvatarType type) {
        switch (type) {
        case UniverseAvatarType::MorphToR6: return "R6";
        case UniverseAvatarType::MorphToR15: return "R15";
        default: return "Player Choice";
        }
    }

    constexpr int UniverseAccessTypeCount = 3;
    enum class UniverseAccessType { Public = 0, Private = 1, FriendsOnly = 2 };

    inline const char* UniverseAccessTypeAsTranslatableString(UniverseAccessType type) {
        switch (type) {
        case UniverseAccessType::Private: return "Private";
        case UniverseAccessType::FriendsOnly: return "Friends Only";
        default: return "Public";
        }
    }

    constexpr int UniversePaymentTypeCount = 2;
    enum class UniversePaymentType { Free = 0, PaidAccess = 1 };

    inline const char* UniversePaymentTypeAsTranslatableString(UniversePaymentType type) {
        switch (type) {
        case UniversePaymentType::PaidAccess: return "Paid Access";
        default: return "Free";
        }
    }

    constexpr int AgeRatingCount = 5;
    enum class AgeRating { Unspecified = 0, AllAges = 1, NinePlus = 2, ThirteenPlus = 3, SeventeenPlus = 4 };

    inline const char* AgeRatingAsTranslatableString(AgeRating rating) {
        switch (rating) {
        case AgeRating::AllAges: return "All Ages";
        case AgeRating::NinePlus: return "9+";
        case AgeRating::ThirteenPlus: return "13+";
        case AgeRating::SeventeenPlus: return "17+";
        default: return "Unspecified";
        }
    }

    // Mirrors Roblox Enum.SocialLinkType for a universe's social link.
    constexpr int SocialLinkTypeCount = 7;
    enum class SocialLinkType {
        Facebook = 0, Twitter = 1, YouTube = 2, Twitch = 3,
        Discord = 4, RobloxGroup = 5, Guilded = 6
    };

    inline const char* SocialLinkTypeAsTranslatableString(SocialLinkType type) {
        switch (type) {
        case SocialLinkType::Twitter: return "Twitter";
        case SocialLinkType::YouTube: return "YouTube";
        case SocialLinkType::Twitch: return "Twitch";
        case SocialLinkType::Discord: return "Discord";
        case SocialLinkType::RobloxGroup: return "Roblox Group";
        case SocialLinkType::Guilded: return "Guilded";
        default: return "Facebook";
        }
    }
}