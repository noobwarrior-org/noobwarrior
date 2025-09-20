// === noobWarrior ===
// File: Set.h (Database)
// Started by: Hattozo
// Started on: 9/20/2025
// Description: A C-struct representation of what the database would give you when you ask it for a set.
#pragma once
#include <NoobWarrior/Database/Record/OwnedIdRecord.h>
#include <NoobWarrior/Database/Record/IdType/Asset.h>

namespace NoobWarrior {
struct Set : OwnedIdRecord {
    std::vector<Asset>          Items;

    static constexpr const char* TableName = "Set";
};
}
