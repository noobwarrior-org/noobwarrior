// === noobWarrior ===
// File: Bundle.h (Database)
// Started by: Hattozo
// Started on: 9/20/2025
// Description:
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

    static constexpr std::string TypeName = "Bundle";
};
}
