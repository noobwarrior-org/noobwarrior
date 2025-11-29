// === noobWarrior ===
// File: DevProduct.h (Database)
// Started by: Hattozo
// Started on: 9/20/2025
// Description: A C-struct representation of what the database would give you when you ask it for a developer product.
#pragma once
#include <NoobWarrior/Database/Item/UniverseItem.h>

namespace NoobWarrior {
struct DevProduct : UniverseItem {
    Roblox::CurrencyType        CurrencyType;
    int                         Price;
};
}
