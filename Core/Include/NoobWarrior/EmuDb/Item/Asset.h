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
// File: Asset.h (Database)
// Started by: Hattozo
// Started on: 6/30/2025
// Description:
#pragma once
#include <NoobWarrior/EmuDb/Item/StatisticalItem.h>
#include <NoobWarrior/Roblox/Api/Asset.h>
#include <NoobWarrior/Roblox/Api/User.h>
#include <NoobWarrior/Roblox/Api/Gear.h>

#include <nlohmann/json.hpp>

#include <cstdint>
#include <vector>

namespace NoobWarrior {
struct Asset : StatisticalItem {
    Roblox::AssetType                               Type;

    // If set for "Place" type, it will make it uncopylocked.
    // For development items, it will make it free to take.
    // For avatar items, it will make it on-sale.
    bool                             Public; // Uncopylocked, or free-to-take development asset?
    
    // "Place" type only
    int                              MaxPlayers;
    bool                             AllowDirectAccess;
    nlohmann::json                   Thumbnails;
    Roblox::GearGenreSetting         GearGenrePermission;
    std::vector<Roblox::GearType>    GearTypes;

    // All fields below only apply to avatar items.
    Roblox::CurrencyType             CurrencyType;
    int                              Price;
    bool                             IsNew;
    Roblox::LimitedType              LimitedType;
    int64_t                          Remaining;
    int                              ContentRatingTypeId;
    int                              MinimumMembershipLevel;

    static constexpr std::string TypeName = "Asset";
};

constexpr int AssetCategoryCount = 1;
enum class AssetCategory {
    DevelopmentItem,
    AvatarItem
};

inline AssetCategory MapAssetTypeToCategory(Roblox::AssetType type) {
    switch (type) {
    default: return AssetCategory::DevelopmentItem;
    case Roblox::AssetType::Image: return AssetCategory::DevelopmentItem;
    case Roblox::AssetType::TShirt: return AssetCategory::AvatarItem;
    case Roblox::AssetType::Audio: return AssetCategory::DevelopmentItem;
    case Roblox::AssetType::Mesh: return AssetCategory::DevelopmentItem;
    case Roblox::AssetType::Lua: return AssetCategory::DevelopmentItem;
    case Roblox::AssetType::Hat: return AssetCategory::AvatarItem;
    case Roblox::AssetType::Place: return AssetCategory::DevelopmentItem;
    case Roblox::AssetType::Model: return AssetCategory::DevelopmentItem;
    case Roblox::AssetType::Shirt: return AssetCategory::AvatarItem;
    case Roblox::AssetType::Pants: return AssetCategory::AvatarItem;
    case Roblox::AssetType::Decal: return AssetCategory::DevelopmentItem;
    case Roblox::AssetType::Head: return AssetCategory::AvatarItem;
    case Roblox::AssetType::Face: return AssetCategory::AvatarItem;
    case Roblox::AssetType::Gear: return AssetCategory::AvatarItem;
    case Roblox::AssetType::Badge: return AssetCategory::DevelopmentItem;
    case Roblox::AssetType::Animation: return AssetCategory::DevelopmentItem;
    case Roblox::AssetType::Torso: return AssetCategory::AvatarItem;
    case Roblox::AssetType::RightArm: return AssetCategory::AvatarItem;
    case Roblox::AssetType::LeftArm: return AssetCategory::AvatarItem;
    case Roblox::AssetType::LeftLeg: return AssetCategory::AvatarItem;
    case Roblox::AssetType::RightLeg: return AssetCategory::AvatarItem;
    case Roblox::AssetType::Package: return AssetCategory::AvatarItem;
    case Roblox::AssetType::Gamepass: return AssetCategory::DevelopmentItem;
    case Roblox::AssetType::Plugin: return AssetCategory::DevelopmentItem;
    case Roblox::AssetType::MeshPart: return AssetCategory::DevelopmentItem;
    case Roblox::AssetType::HairAccessory: return AssetCategory::AvatarItem;
    case Roblox::AssetType::FaceAccessory: return AssetCategory::AvatarItem;
    case Roblox::AssetType::NeckAccessory: return AssetCategory::AvatarItem;
    case Roblox::AssetType::ShoulderAccessory: return AssetCategory::AvatarItem;
    case Roblox::AssetType::FrontAccessory: return AssetCategory::AvatarItem;
    case Roblox::AssetType::BackAccessory: return AssetCategory::AvatarItem;
    case Roblox::AssetType::WaistAccessory: return AssetCategory::AvatarItem;
    case Roblox::AssetType::ClimbAnimation: return AssetCategory::AvatarItem;
    case Roblox::AssetType::DeathAnimation: return AssetCategory::AvatarItem;
    case Roblox::AssetType::FallAnimation: return AssetCategory::AvatarItem;
    case Roblox::AssetType::IdleAnimation: return AssetCategory::AvatarItem;
    case Roblox::AssetType::JumpAnimation: return AssetCategory::AvatarItem;
    case Roblox::AssetType::RunAnimation: return AssetCategory::AvatarItem;
    case Roblox::AssetType::SwimAnimation: return AssetCategory::AvatarItem;
    case Roblox::AssetType::WalkAnimation: return AssetCategory::AvatarItem;
    case Roblox::AssetType::PoseAnimation: return AssetCategory::AvatarItem;
    case Roblox::AssetType::EarAccessory: return AssetCategory::AvatarItem;
    case Roblox::AssetType::EyeAccessory: return AssetCategory::AvatarItem;
    case Roblox::AssetType::EmoteAnimation: return AssetCategory::AvatarItem;
    case Roblox::AssetType::Video: return AssetCategory::DevelopmentItem;
    case Roblox::AssetType::TShirtAccessory: return AssetCategory::AvatarItem;
    case Roblox::AssetType::ShirtAccessory: return AssetCategory::AvatarItem;
    case Roblox::AssetType::PantsAccessory: return AssetCategory::AvatarItem;
    case Roblox::AssetType::JacketAccessory: return AssetCategory::AvatarItem;
    case Roblox::AssetType::SweaterAccessory: return AssetCategory::AvatarItem;
    case Roblox::AssetType::ShortsAccessory: return AssetCategory::AvatarItem;
    case Roblox::AssetType::LeftShoeAccessory: return AssetCategory::AvatarItem;
    case Roblox::AssetType::RightShoeAccessory: return AssetCategory::AvatarItem;
    case Roblox::AssetType::DressSkirtAccessory: return AssetCategory::AvatarItem;
    case Roblox::AssetType::FontFamily: return AssetCategory::DevelopmentItem;
    case Roblox::AssetType::EyebrowAccessory: return AssetCategory::AvatarItem;
    case Roblox::AssetType::EyelashAccessory: return AssetCategory::AvatarItem;
    case Roblox::AssetType::MoodAnimation: return AssetCategory::AvatarItem;
    case Roblox::AssetType::DynamicHead: return AssetCategory::AvatarItem;
    }
}

inline const char *AssetCategoryAsTranslatableString(AssetCategory type) {
    switch (type) {
        case AssetCategory::DevelopmentItem: return "Development Item";
        case AssetCategory::AvatarItem: return "Avatar Item";
        default: return "Unknown";
    }
}

constexpr int AvatarCategoryCount = 3;
enum class AvatarCategory {
    None,
    Accessory,
    Animation,
    Package
};
}
