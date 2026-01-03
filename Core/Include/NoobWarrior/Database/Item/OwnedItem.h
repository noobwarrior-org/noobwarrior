// === noobWarrior ===
// File: OwnedItem.h
// Started by: Hattozo
// Started on: N/A
// Description:
#pragma once
#include "Item.h"
#include <NoobWarrior/Roblox/Api/Asset.h>

#include <optional>

namespace NoobWarrior {
struct OwnedItem : Item {
    Roblox::CreatorType CreatorType;
    int64_t             CreatorId;
    std::string         Description;
    int64_t             Created;
    int64_t             Updated;
    int64_t             ImageId;
    int                 ImageSnapshot;
};
}
