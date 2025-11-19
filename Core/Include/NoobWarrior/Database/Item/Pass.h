// === noobWarrior ===
// File: Pass.h (Database)
// Started by: Hattozo
// Started on: 9/20/2025
// Description: A C-struct representation of what the database would give you when you ask it for a pass.
#pragma once
#include <NoobWarrior/Database/Item/UniverseItem.h>

namespace NoobWarrior {
struct Pass : UniverseItem {
    Roblox::CurrencyType        CurrencyType;
    int                         Price;
    bool                        IsForSale;
};
}
