// === noobWarrior ===
// File: Asset.h (Database)
// Started by: Hattozo
// Started on: 6/30/2025
// Description:
// Please note: All nullable values should be wrapped in std::optional, for the sake of everyone's sanity.
// Doesn't matter if it's a integer or whatever. As long as there's no NOT NULL clause in the schema, wrap its C++ equivalent in std::optional. Please.
#pragma once
#include <NoobWarrior/Database/Item/StatisticalItem.h>
#include <NoobWarrior/Roblox/Api/Asset.h>
#include <NoobWarrior/Roblox/Api/User.h>

#include <nlohmann/json.hpp>

#include <cstdint>

namespace NoobWarrior {
struct Asset : StatisticalItem {
    Roblox::AssetType           Type;
    nlohmann::json              Thumbnails;
    std::optional<Roblox::CurrencyType>        CurrencyType;
    std::optional<int>                         Price;
    std::optional<bool>                        Public;
    std::optional<bool>                        IsNew;
    std::optional<Roblox::LimitedType>         LimitedType;
    std::optional<int64_t>                     Remaining;
    std::optional<int>                         ContentRatingTypeId;
    std::optional<int>                         MinimumMembershipLevel;
};
}
