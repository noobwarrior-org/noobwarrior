// === noobWarrior ===
// File: Set.h (Database)
// Started by: Hattozo
// Started on: 9/20/2025
// Description: A C-struct representation of what the database would give you when you ask it for a set.
#pragma once
#include <NoobWarrior/Database/Item/OwnedItem.h>
#include <NoobWarrior/Database/Item/Asset.h>

namespace NoobWarrior {
struct Set : OwnedItem {
    std::vector<Asset>          Items;
};
}
