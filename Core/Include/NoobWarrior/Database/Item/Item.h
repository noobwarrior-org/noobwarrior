// === noobWarrior ===
// File: Item.h
// Started by: Hattozo
// Started on: 11/11/2025
// Description: Base class for all items (assets, groups, games, users)
#pragma once
#include <cstdint>
#include <string>

#include "../ContentImages.h"

namespace NoobWarrior {
struct Item {
    int64_t             Id;
    int                 Snapshot;
    int64_t             FirstRecorded;
    int64_t             LastRecorded;
    std::string         Name;

    static constexpr std::string TypeName = "Item";
    static constexpr const int* DefaultImage = g_icon_content_deleted;
    inline static const int DefaultImageSize = g_icon_content_deleted_size;
};
}
