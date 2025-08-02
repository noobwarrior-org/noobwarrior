// === noobWarrior ===
// File: Asset.h (Database)
// Started by: Hattozo
// Started on: 6/30/2025
// Description: A C-struct representation of what the database would give you when you ask it for an asset.
#pragma once
#include "../Record/UniverseIdRecord.h"

namespace NoobWarrior {
struct Badge : UniverseIdRecord {
    int Awarded;
    int AwardedYesterday;

    static constexpr const char* TableName = "Badge";
};
}