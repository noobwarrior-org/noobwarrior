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
// File: ToolboxAssetCategory.h
// Started by: Hattozo
// Started on: 6/6/2026
// Description: Maps toolbox category names (modern "Models"/"Audio" and legacy "FreeModels"/...) to asset types.
#pragma once
#include <NoobWarrior/Roblox/Api/Asset.h>

#include <cctype>
#include <string>

namespace NoobWarrior {
// Resolves a toolbox category path/param to the asset type it lists. Accepts the modern plural
// names (Models, Decals, Audio, Meshes, Plugins, Videos, Images, Animations) and the legacy
// Free* names. Anything unrecognised maps to AssetType::None, which matches any asset type.
inline Roblox::AssetType ToolboxCategoryToAssetType(std::string category) {
    for (char &c : category)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (category == "models" || category == "model" || category == "freemodels")
        return Roblox::AssetType::Model;
    if (category == "decals" || category == "decal" || category == "freedecals")
        return Roblox::AssetType::Decal;
    if (category == "audio" || category == "audios" || category == "freeaudio" || category == "sounds")
        return Roblox::AssetType::Audio;
    if (category == "meshes" || category == "mesh" || category == "meshparts" || category == "meshpart")
        return Roblox::AssetType::MeshPart;
    if (category == "plugins" || category == "plugin")
        return Roblox::AssetType::Plugin;
    if (category == "videos" || category == "video")
        return Roblox::AssetType::Video;
    if (category == "images" || category == "image")
        return Roblox::AssetType::Image;
    if (category == "animations" || category == "animation")
        return Roblox::AssetType::Animation;

    return Roblox::AssetType::None;
}
}
