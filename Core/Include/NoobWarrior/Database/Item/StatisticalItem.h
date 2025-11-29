// === noobWarrior ===
// File: StatisticalItem.h
// Started by: Hattozo
// Started on: N/A
// Description:
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
    int64_t                     Historical_Sales;
    int64_t                     Historical_Favorites;
    int64_t                     Historical_Likes;
    int64_t                     Historical_Dislikes;
};
}
