// === noobWarrior ===
// File: Asset.h (Database)
// Started by: Hattozo
// Started on: 6/30/2025
// Description: A C-struct representation of what the database would give you when you ask it for an asset.
#pragma once
#include "../Record/OwnedIdRecord.h"
#include "../../Roblox/Api/Asset.h"
#include "../../Roblox/Api/User.h"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>

namespace NoobWarrior {
struct Asset : OwnedIdRecord {
    Roblox::AssetType           Type;
    nlohmann::json              Thumbnails;
    int                         PriceInRobux;
    int                         PriceInTickets;
    int                         ContentRatingTypeId;
    int                         MinimumMembershipLevel;
    bool                        IsPublicDomain;
    bool                        IsForSale;
    bool                        IsNew;
    Roblox::LimitedType         LimitedType;
    int64_t                     Remaining;
    int64_t                     Sales;
    int64_t                     Favorites;
    int64_t                     Likes;
    int64_t                     Dislikes;
    std::vector<unsigned char>  Data;

    static constexpr const char* TableName = "Asset";
};
}
