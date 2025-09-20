// === noobWarrior ===
// File: StatisticalIdRecord.h
// Started by: Hattozo
// Started on: N/A
// Description:
#pragma once
#include "OwnedIdRecord.h"

namespace NoobWarrior {
struct StatisticalIdRecord : OwnedIdRecord {
    /* These numbers are meant for viewing only and cannot be modified through AddContent(). */
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
