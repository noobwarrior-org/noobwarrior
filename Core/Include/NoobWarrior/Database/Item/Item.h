// === noobWarrior ===
// File: Item.h
// Started by: Hattozo
// Started on: 11/11/2025
// Description: Base class for all items (assets, groups, games, users)
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
