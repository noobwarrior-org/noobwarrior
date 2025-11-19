// === noobWarrior ===
// File: Item.h
// Started by: Hattozo
// Started on: 11/11/2025
// Description: Base class for all items (assets, groups, games, users)
// Please note: All nullable values should be wrapped in std::optional, for the sake of everyone's sanity.
// Doesn't matter if it's a integer or whatever. As long as there's no NOT NULL clause in the schema, wrap its C++ equivalent in std::optional. Please.
#pragma once
#include <cstdint>
#include <string>

namespace NoobWarrior {
struct Item {
    int64_t             Id;
    int                 Snapshot;
    int64_t             FirstRecorded;
    int64_t             LastRecorded;
    std::string         Name;
};
}
