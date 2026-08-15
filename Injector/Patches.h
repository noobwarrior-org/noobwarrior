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
// File: Patches.h
// Started by: Hattozo
// Started on: 8/12/2025
// Description: Per-version Hyperion patch offset tables.
// This is unused. LOL!
// Slopped by GPT 5.6 Sol and DeepSeek V4 Pro
#pragma once

#include "Bypass.h"

#include <vector>

// Player 0.719 uses the loader-free, image-backed WSP redirect and does not
// modify Hyperion code pages. Older Players do not require Hyperion patches.
inline std::vector<PatchEntry> GetPatchesForEra(RobloxEra) {
    return {};
}
