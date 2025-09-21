// === noobWarrior ===
// File: Bundle.h (Database)
// Started by: Hattozo
// Started on: 9/20/2025
// Description: A C-struct representation of what the database would give you when you ask it for a bundle.
#pragma once
#include <NoobWarrior/Database/Record/StatisticalIdRecord.h>
#include <NoobWarrior/Database/Record/IdType/Asset.h>
#include <NoobWarrior/Roblox/Api/Bundle.h>

#include <nlohmann/json.hpp>

#include <cstdint>

namespace NoobWarrior {
struct Bundle : StatisticalIdRecord {
    Roblox::BundleType          Type;
    Roblox::CurrencyType        CurrencyType;
    int                         Price;
    int                         ContentRatingTypeId;
    int                         MinimumMembershipLevel;
    bool                        IsPublicDomain;
    bool                        IsForSale;
    bool                        IsNew;
    Roblox::LimitedType         LimitedType;
    int64_t                     Remaining;

    /* This cannot be modified through AddContent(), please find another way. */
    std::vector<Asset>          Items;
};
}
