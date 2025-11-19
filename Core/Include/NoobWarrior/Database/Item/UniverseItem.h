// === noobWarrior ===
// File: UniverseItem.h
// Started by: Hattozo
// Started on: 7/31/2025
// Description:
// Please note: All nullable values should be wrapped in std::optional, for the sake of everyone's sanity.
// Doesn't matter if it's a integer or whatever. As long as there's no NOT NULL clause in the schema, wrap its C++ equivalent in std::optional. Please.
#pragma once
#include "OwnedItem.h"

namespace NoobWarrior {
struct UniverseItem : OwnedItem {
    int64_t UniverseId;
};
}
