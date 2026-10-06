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
// File: ScreenRecord.h
// Started by: Hattozo
// Started on: 10/6/2026
// Description: Provides cross-platform API for screen shotting and recording
#pragma once
#include <vector>
#include <string>
#include <optional>

namespace NoobWarrior {
struct Screenshot {
    std::vector<unsigned char> Pixels;
    size_t Width;
    size_t Height;
};

struct ScreenshotOpts {
    std::string Destintation {}; // If empty, will return in memory
};

std::optional<Screenshot> CaptureScreen(ScreenshotOpts opts);
}