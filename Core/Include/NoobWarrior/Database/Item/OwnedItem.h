// === noobWarrior ===
// File: OwnedItem.h
// Started by: Hattozo
// Started on: N/A
// Description:
// Please note: All nullable values should be wrapped in std::optional, for the sake of everyone's sanity.
// Doesn't matter if it's a integer or whatever. As long as there's no NOT NULL clause in the schema, wrap its C++ equivalent in std::optional. Please.
#pragma once
#include "Item.h"
#include <NoobWarrior/Roblox/Api/Asset.h>

#include <optional>

namespace NoobWarrior {
struct OwnedItem : Item {
    Roblox::CreatorType CreatorType;
    std::optional<int64_t>             CreatorId;
    std::optional<std::string>         Description;
    std::optional<int64_t>             Created;
    std::optional<int64_t>             Updated;
    std::optional<int64_t>             ImageId;
    std::optional<int>                 ImageSnapshot;
};
}
