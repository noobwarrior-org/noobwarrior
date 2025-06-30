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
    enum class AssetType { Image = 1, TShirt = 2, Audio = 3, Mesh = 4, Lua = 5,
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
}