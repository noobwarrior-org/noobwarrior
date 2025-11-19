// === noobWarrior ===
// File: DevProduct.h (Database)
// Started by: Hattozo
// Started on: 9/20/2025
// Description: A C-struct representation of what the database would give you when you ask it for a developer product.
// Please note: All nullable values should be wrapped in std::optional, for the sake of everyone's sanity.
// Doesn't matter if it's a integer or whatever. As long as there's no NOT NULL clause in the schema, wrap its C++ equivalent in std::optional. Please.
#pragma once
#include <NoobWarrior/Database/Item/UniverseItem.h>

namespace NoobWarrior {
struct DevProduct : UniverseItem {
    Roblox::CurrencyType        CurrencyType;
    int                         Price;
};
}
