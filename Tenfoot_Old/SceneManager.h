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
// File: SceneManager.h
// Started by: Hattozo
// Started on: 6/17/2025
// Description:
#pragma once
#include "Scene.h"
#include <blend2d/blend2d.h>

namespace NoobWarrior {
class SceneManager {
public:
    SceneManager();
    void RedrawImage(int width, int height, int64_t ticks);
    void SetScene(Scene& scene);

    int GetWidth();
    int GetHeight();
    int GetStride();
    void *GetPixelData();
private:
    BLImage Image;
    BLContext Context;
    BLImageData ImageData;
};
}
