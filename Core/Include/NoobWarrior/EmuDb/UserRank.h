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
// File: UserRank.h
// Started by: Hattozo
// Started on: 8/23/2026
// Description:
#pragma once

#include <cstdint>

namespace NoobWarrior {
inline constexpr int64_t kUserRankMin = 0;
inline constexpr int64_t kUserRankMax = 255;

inline constexpr int64_t kUserRankGuest     = 0;
inline constexpr int64_t kUserRankMember    = 1;
inline constexpr int64_t kUserRankModerator = 100;
inline constexpr int64_t kUserRankAdmin     = 200;
inline constexpr int64_t kUserRankOwner     = 255;

inline constexpr int64_t kUserRankDefault = kUserRankMember;

inline constexpr bool UserRankBypassesPermissions(int64_t rank) {
    return rank >= kUserRankMax;
}

inline constexpr bool IsValidUserRank(int64_t rank) {
    return rank >= kUserRankMin && rank <= kUserRankMax;
}

inline constexpr int64_t ClampUserRank(int64_t rank) {
    if (rank < kUserRankMin)
        return kUserRankMin;
    if (rank > kUserRankMax)
        return kUserRankMax;
    return rank;
}
}
