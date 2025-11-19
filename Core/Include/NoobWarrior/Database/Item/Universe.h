// === noobWarrior ===
// File: Universe.h (Database)
// Started by: Hattozo
// Started on: 8/1/2025
// Description: A C-struct representation of what the database would give you when you ask it for a universe.
// Please note: All nullable values should be wrapped in std::optional, for the sake of everyone's sanity.
// Doesn't matter if it's a integer or whatever. As long as there's no NOT NULL clause in the schema, wrap its C++ equivalent in std::optional. Please.
#pragma once
#include <NoobWarrior/Database/Item/OwnedItem.h>

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>

namespace NoobWarrior {
struct Universe : OwnedItem {
    
};
}
