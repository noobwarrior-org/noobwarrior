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
// File: ContentImages.h
// Started by: Hattozo
// Started on: 8/2/2025
// Description: Handles everything relating to images used for id and asset types.
// The external variables are implemented in the respective source file, specifically ContentImages.cpp
#pragma once
#include "NoobWarrior/Roblox/Api/Asset.h"
#include <vector>

extern const int g_icon_content_deleted[];
extern const int g_icon_content_deleted_size;

extern const int g_question_mark_png[];
extern const int g_question_mark_png_size;

extern const int g_database_png[];
extern const int g_database_png_size;

extern const int g_model_png[];
extern const int g_model_png_size;

extern const int g_audio_png[];
extern const int g_audio_png_size;

extern const int g_animation_png[];
extern const int g_animation_png_size;

namespace NoobWarrior {
inline std::vector<unsigned char> GetImageForAssetType(Roblox::AssetType assetType) {
    std::vector<unsigned char> data;
    const int *icon = g_question_mark_png;
    int icon_size = g_question_mark_png_size;
    switch (assetType) {
    default: break;
    case Roblox::AssetType::Model: icon = g_model_png; icon_size = g_model_png_size; break;
    case Roblox::AssetType::Audio: icon = g_audio_png; icon_size = g_audio_png_size; break;
    case Roblox::AssetType::Animation: icon = g_animation_png; icon_size = g_animation_png_size; break;
    }
    data.assign(icon, icon + icon_size);
    return data;
}
}
