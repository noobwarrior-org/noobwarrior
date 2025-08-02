// === noobWarrior ===
// File: AssetCategory.h
// Started by: Hattozo
// Started on: 7/25/2025
// Description: A more generalized version of the AssetType enum
#pragma once
#include "../Roblox/Api/Asset.h"

namespace NoobWarrior {
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
}