// === noobWarrior ===
// File: StatisticalItem.h
// Started by: Hattozo
// Started on: N/A
// Description:
// Please note: All nullable values should be wrapped in std::optional, for the sake of everyone's sanity.
// Doesn't matter if it's a integer or whatever. As long as there's no NOT NULL clause in the schema, wrap its C++ equivalent in std::optional. Please.
#pragma once
#include "OwnedItem.h"

namespace NoobWarrior {
struct StatisticalItem : OwnedItem {
    /* Because this data is computed in real-time, these numbers are meant for viewing only and cannot be modified by simply editing these properties. */
    int64_t                     Sales;
    int64_t                     Favorites;
    int64_t                     Likes;
    int64_t                     Dislikes;

    /* This however, can be modified. */
    std::optional<int64_t>                     Historical_Sales;
    std::optional<int64_t>                     Historical_Favorites;
    std::optional<int64_t>                     Historical_Likes;
    std::optional<int64_t>                     Historical_Dislikes;
};
}
