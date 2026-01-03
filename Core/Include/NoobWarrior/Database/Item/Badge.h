// === noobWarrior ===
// File: Badge.h (Database)
// Started by: Hattozo
// Started on: 6/30/2025
// Description:
#pragma once
#include <NoobWarrior/Database/Item/UniverseItem.h>

namespace NoobWarrior {
struct Badge : UniverseItem {
    int Awarded;
    int AwardedYesterday;

    static constexpr std::string TypeName = "Badge";
};
}