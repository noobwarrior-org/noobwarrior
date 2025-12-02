// === noobWarrior ===
// File: Universe.h (Database)
// Started by: Hattozo
// Started on: 8/1/2025
// Description: A C-struct representation of what the database would give you when you ask it for a universe.
#pragma once
#include <NoobWarrior/Database/Item/OwnedItem.h>

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>

namespace NoobWarrior {
struct Universe : OwnedItem {
    static constexpr std::string TypeName = "Universe";
};
}
