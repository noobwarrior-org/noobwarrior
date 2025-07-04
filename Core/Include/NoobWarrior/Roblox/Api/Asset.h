// === noobWarrior ===
// File: Asset.h (Roblox API)
// Started by: Hattozo
// Started on: 3/15/2025
// Description: A C-struct representation of what the Roblox API would give you when you ask it for an asset.
#pragma once

#include <NoobWarrior/Roblox/Api/User.h>

#include <cstdint>
#include <string>

namespace NoobWarrior::Roblox {
    // Note: "None" is a fallback if the asset does not have a type. It's not a real value in the Roblox API
    enum class AssetType { None = 0, Image = 1, TShirt = 2, Audio = 3, Mesh = 4, Lua = 5,
        Hat = 8, Place = 9, Model = 10, Shirt = 11, Pants = 12,
        Decal = 13, Head = 17, Face = 18, Gear = 19, Badge = 21,
        Animation = 24, Torso = 27, RightArm = 28, LeftArm = 29,
        LeftLeg = 30, RightLeg = 31, Package = 32, Gamepass = 34,
        Plugin = 38, MeshPart = 40, HairAccessory = 41, FaceAccessory = 42,
        NeckAccessory = 43, ShoulderAccessory = 44, FrontAccessory = 45,
        BackAccessory = 46, WaistAccessory = 47, ClimbAnimation = 48,
        DeathAnimation = 49, FallAnimation = 50, IdleAnimation = 51,
        JumpAnimation = 52, RunAnimation = 53, SwimAnimation = 54,
        WalkAnimation = 55, PoseAnimation = 56, EarAccessory = 57,
        EyeAccessory = 58, EmoteAnimation = 61, Video = 62, TShirtAccessory = 64,
        ShirtAccessory = 65, PantsAccessory = 66, JacketAccessory = 67,
        SweaterAccessory = 68, ShortsAccessory = 69, LeftShoeAccessory = 70,
        RightShoeAccessory = 71, DressSkirtAccessory = 72, FontFamily = 73,
        EyebrowAccessory = 76, EyelashAccessory = 77, MoodAnimation = 78,
        DynamicHead = 79 };

    enum class ProductType {
        None,
        UserProduct,
        CollectibleItem
    };

    enum class LimitedType {
        None,
        Limited,
        LimitedUnique
    };
    
    struct AssetDetails {
        int64_t             TargetId                {};
        Roblox::ProductType ProductType             {};
        int64_t             AssetId                 {};
        int64_t             ProductId               {};
        std::string         Name                    {};
        std::string         Description             {};
        Roblox::AssetType   AssetType               {};
        int64_t             CreatorId               {};
        std::string         CreatorName             {};
        Roblox::CreatorType CreatorType             {};
        int64_t             CreatorTargetId         {};
        bool                CreatorHasVerifiedBadge {};
        int64_t             IconImageAssetId        {};
        long                Created                 {};
        long                Updated                 {};
        int64_t             PriceInRobux            {};
        int64_t             PriceInTickets          {};
        int64_t             Sales                   {};
        bool                IsNew                   {};
        bool                IsForSale               {};
        bool                IsPublicDomain          {};
        bool                IsLimited               {};
        bool                IsLimitedUnique         {};
        MembershipType      MinimumMembershipLevel  {};
    };

    inline const char *AssetTypeAsTranslatableString(AssetType type) {
        switch (type) {
        case AssetType::Image: return "Image";
        case AssetType::TShirt: return "TShirt";
        case AssetType::Audio: return "Audio";
        case AssetType::Mesh: return "Mesh";
        case AssetType::Lua: return "Lua";
        case AssetType::Hat: return "Hat";
        case AssetType::Place: return "Place";
        case AssetType::Model: return "Model";
        case AssetType::Shirt: return "Shirt";
        case AssetType::Pants: return "Pants";
        case AssetType::Decal: return "Decal";
        case AssetType::Head: return "Head";
        case AssetType::Face: return "Face";
        case AssetType::Gear: return "Gear";
        case AssetType::Badge: return "Badge";
        case AssetType::Animation: return "Animation";
        case AssetType::Torso: return "Torso";
        case AssetType::RightArm: return "RightArm";
        case AssetType::LeftArm: return "LeftArm";
        case AssetType::LeftLeg: return "LeftLeg";
        case AssetType::RightLeg: return "RightLeg";
        case AssetType::Package: return "Package";
        case AssetType::Gamepass: return "Gamepass";
        case AssetType::Plugin: return "Plugin";
        case AssetType::MeshPart: return "MeshPart";
        case AssetType::HairAccessory: return "Hair Accessory";
        case AssetType::FaceAccessory: return "Face Accessory";
        case AssetType::NeckAccessory: return "Neck Accessory";
        case AssetType::ShoulderAccessory: return "Shoulder Accessory";
        case AssetType::FrontAccessory: return "Front Accessory";
        case AssetType::BackAccessory: return "Back Accessory";
        case AssetType::WaistAccessory: return "Waist Accessory";
        case AssetType::ClimbAnimation: return "Climb Animation";
        case AssetType::DeathAnimation: return "Death Animation";
        case AssetType::FallAnimation: return "Fall Animation";
        case AssetType::IdleAnimation: return "Idle Animation";
        case AssetType::JumpAnimation: return "Jump Animation";
        case AssetType::RunAnimation: return "Run Animation";
        case AssetType::SwimAnimation: return "Swim Animation";
        case AssetType::WalkAnimation: return "Walk Animation";
        case AssetType::PoseAnimation: return "Pose Animation";
        case AssetType::EarAccessory: return "Ear Accessory";
        case AssetType::EyeAccessory: return "Eye Accessory";
        case AssetType::EmoteAnimation: return "Emote Animation";
        case AssetType::Video: return "Video";
        case AssetType::TShirtAccessory: return "T-Shirt Accessory";
        case AssetType::ShirtAccessory: return "Shirt Accessory";
        case AssetType::PantsAccessory: return "Pants Accessory";
        case AssetType::JacketAccessory: return "Jacket Accessory";
        case AssetType::SweaterAccessory: return "Sweater Accessory";
        case AssetType::ShortsAccessory: return "Shorts Accessory";
        case AssetType::LeftShoeAccessory: return "Left Shoe Accessory";
        case AssetType::RightShoeAccessory: return "Right Shoe Accessory";
        case AssetType::DressSkirtAccessory: return "Dress Skirt Accessory";
        case AssetType::FontFamily: return "Font Family";
        case AssetType::EyebrowAccessory: return "Eyebrow Accessory";
        case AssetType::EyelashAccessory: return "Eyelash Accessory";
        case AssetType::MoodAnimation: return "Mood Animation";
        case AssetType::DynamicHead: return "Dynamic Head";
        default: return "None";
        }
    }
}