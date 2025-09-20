// === noobWarrior ===
// File: Pass.h (Database)
// Started by: Hattozo
// Started on: 9/20/2025
// Description: A C-struct representation of what the database would give you when you ask it for a pass.
#pragma once
#include <NoobWarrior/Database/Record/UniverseIdRecord.h>

namespace NoobWarrior {
struct Pass : UniverseIdRecord {
    Roblox::CurrencyType        CurrencyType;
    int                         Price;
    bool                        IsForSale;

    static constexpr const char* TableName = "Pass";
};
}
