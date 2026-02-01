/*
 * Copyright (C) 2026 Hattozo
 *
 * This file is part of noobWarrior.
 *
 * noobWarrior is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * noobWarrior is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with noobWarrior; if not, see
 * <https://www.gnu.org/licenses/>.
 */
// === noobWarrior ===
// File: Bundle.h (Database)
// Started by: Hattozo
// Started on: 9/20/2025
// Description:
#pragma once
#include <NoobWarrior/EmuDb/Item/StatisticalItem.h>
#include <NoobWarrior/EmuDb/Item/Asset.h>
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
