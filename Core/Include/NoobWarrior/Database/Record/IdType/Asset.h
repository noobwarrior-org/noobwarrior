// === noobWarrior ===
// File: Asset.h (Database)
// Started by: Hattozo
// Started on: 6/30/2025
// Description: A C-struct representation of what the database would give you when you ask it for an asset.
#pragma once
#include <NoobWarrior/Database/Record/StatisticalIdRecord.h>
#include <NoobWarrior/Roblox/Api/Asset.h>
#include <NoobWarrior/Roblox/Api/User.h>

#include <nlohmann/json.hpp>

#include <cstdint>

namespace NoobWarrior {
struct Asset : StatisticalIdRecord {
    Roblox::AssetType           Type;
    nlohmann::json              Thumbnails;
    Roblox::CurrencyType        CurrencyType;
    int                         Price;
    int                         ContentRatingTypeId;
    int                         MinimumMembershipLevel;
    bool                        IsPublicDomain;
    bool                        IsForSale;
    bool                        IsNew;
    Roblox::LimitedType         LimitedType;
    int64_t                     Remaining;
};
}
