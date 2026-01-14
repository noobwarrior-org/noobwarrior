/*
 * Copyright (C) 2026 Hattozo
 *
 * This file is part of noobWarrior.
 *
 * noobWarrior is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * noobWarrior is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with noobWarrior; if not, see
 * <https://www.gnu.org/licenses/>.
 */
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
