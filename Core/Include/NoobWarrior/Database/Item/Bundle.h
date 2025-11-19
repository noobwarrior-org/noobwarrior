// === noobWarrior ===
// File: Bundle.h (Database)
// Started by: Hattozo
// Started on: 9/20/2025
// Description:
// Please note: All nullable values should be wrapped in std::optional, for the sake of everyone's sanity.
// Doesn't matter if it's a integer or whatever. As long as there's no NOT NULL clause in the schema, wrap its C++ equivalent in std::optional. Please.
#pragma once
#include <NoobWarrior/Database/Item/StatisticalItem.h>
#include <NoobWarrior/Database/Item/Asset.h>
#include <NoobWarrior/Roblox/Api/Bundle.h>

#include <nlohmann/json.hpp>

#include <cstdint>

namespace NoobWarrior {
struct Bundle : StatisticalItem {
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
