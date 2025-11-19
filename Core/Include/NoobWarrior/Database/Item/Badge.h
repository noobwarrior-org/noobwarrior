// === noobWarrior ===
// File: Badge.h (Database)
// Started by: Hattozo
// Started on: 6/30/2025
// Description:
// Please note: All nullable values should be wrapped in std::optional, for the sake of everyone's sanity.
// Doesn't matter if it's a integer or whatever. As long as there's no NOT NULL clause in the schema, wrap its C++ equivalent in std::optional. Please.
#pragma once
#include <NoobWarrior/Database/Item/UniverseItem.h>

namespace NoobWarrior {
struct Badge : UniverseItem {
    std::optional<int> Awarded;
    std::optional<int> AwardedYesterday;
};
}