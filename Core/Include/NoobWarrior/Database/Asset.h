// === noobWarrior ===
// File: Asset.h (Database)
// Started by: Hattozo
// Started on: 6/30/2025
// Description: A C-struct representation of what the database would give you when you ask it for an asset.
#pragma once
#include "../Roblox/Api/Asset.h"
#include "../Roblox/Api/User.h"

#include <cstdint>
#include <string>

namespace NoobWarrior {
struct Asset {
    int64_t             Id;
    int                 Version;
    int64_t             FirstRecorded;
    int64_t             LastRecorded;
    std::string         Name;
    std::string         Description;
    int64_t             Created;
    int64_t             Updated;
    Roblox::AssetType   Type;
    int64_t             Icon;
    Roblox::CreatorType CreatorType;
    int64_t             CreatorId;
    int                 PriceInRobux;
    int                 PriceInTickets;
    int                 ContentRatingTypeId;
    int                 MinimumMembershipLevel;
    bool                IsPublicDomain;
    bool                IsForSale;
    bool                IsNew;
    Roblox::LimitedType LimitedType;
    int64_t             Remaining;
    int64_t             Sales;
    int64_t             Favorites;
    int64_t             Likes;
    int64_t             Dislikes;
};
}
