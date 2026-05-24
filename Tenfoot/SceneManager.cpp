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
// File: SceneManager.cpp
// Started by: Hattozo
// Started on: 6/17/2025
// Description:
#include "SceneManager.h"

#include <NoobWarrior/Log.h>

using namespace NoobWarrior;

SceneManager::SceneManager() :
    Image(1280, 720, BL_FORMAT_PRGB32),
    Context(Image)
{}

void SceneManager::RedrawImage(int width, int height, int64_t ticks) {
    Context.clear_all();
    // Image.create(width, height, BL_FORMAT_PRGB32);

    BLFontFace face;
    BLResult result = face.create_from_file("tenfoot/fonts/SourceSansPro-Regular.ttf");
    if (result != BL_SUCCESS) {
        Out("Tenfoot::Menu2D", "Failed to load Source Sans Pro font");
        return;
    }

    BLFont font;
    font.create_from_face(face, 50.0f);

    Context.set_fill_style(BLRgba32(0xFFFFFFFF));
    Context.fill_utf8_text(BLPoint(60, 80), font, "noobWarrior");

    Context.rotate(0.785398);
    Context.fill_utf8_text(BLPoint(250, 80), font, "I am rotating");

    Context.end();

    Image.get_data(&ImageData);
}

int SceneManager::GetWidth() {
    return Image.width();
}

int SceneManager::GetHeight() {
    return Image.height();
}

int SceneManager::GetStride() {
    return ImageData.stride;
}

void* SceneManager::GetPixelData() {
    return ImageData.pixel_data;
}
