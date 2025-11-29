// === noobWarrior ===
// File: Reflection.cpp
// Started by: Hattozo
// Started on: 9/19/2025
// Description: Anything that requires runtime to dynamically understand the structure of data
// without having to manually specify it everywhere should go here
//
// ID types for example; you don't want to add checks in 30 different code paths because you added a new ID type.
// Just declare it here and those code paths will automatically discover it.
//
// Note: If you are adding a new ID type, you will still have to manually add glue code between the SQL database schema and
// the actual C structs. Check the Database.cpp & Database.h file in order to see how we did this.
#include "NoobWarrior/Database/Item/StatisticalItem.h"
#include <NoobWarrior/Reflection.h>

#include <NoobWarrior/Database/Database.h>
#include <NoobWarrior/Database/Item/Asset.h>
#include <NoobWarrior/Database/Item/Badge.h>
#include <NoobWarrior/Database/Item/Bundle.h>
#include <NoobWarrior/Database/Item/DevProduct.h>
#include <NoobWarrior/Database/Item/Group.h>
#include <NoobWarrior/Database/Item/Pass.h>
#include <NoobWarrior/Database/Item/Set.h>
#include <NoobWarrior/Database/Item/Universe.h>
#include <NoobWarrior/Database/Item/User.h>

#include <NoobWarrior/Database/ContentImages.h>

#include <entt/entt.hpp>
#include <entt/meta/meta.hpp>

using namespace NoobWarrior;
using namespace entt::literals;

using AssetType = Roblox::AssetType;

void NoobWarrior::Reflection::Register() {
    /////////////// Enums ///////////////
    entt::meta_factory<AssetType>()
        .type("AssetType")
        .data<AssetType::Image>("Image")
        .data<AssetType::TShirt>("TShirt");

    /////////////// Structs/Classes ///////////////
    entt::meta_factory<Item>()
        .type("Item")
        .data<&Item::Id>("Id")
            // .prop("Description"_hs, "The ID of this asset.")
        .data<&Item::Snapshot>("Snapshot")
            // .prop("Description"_hs, "The snapshot number.")
        .data<&Item::FirstRecorded>("FirstRecorded")
        .data<&Item::LastRecorded>("LastRecorded")
        .data<&Item::Name>("Name");

    entt::meta_factory<OwnedItem>()
        .type("OwnedItem")
        .data<&OwnedItem::CreatorType>("CreatorType")
        .data<&OwnedItem::CreatorId>("CreatorId")
        .data<&OwnedItem::Description>("Description")
        .data<&OwnedItem::Created>("Created")
        .data<&OwnedItem::Updated>("Created")
        .data<&OwnedItem::ImageId>("ImageId")
        .data<&OwnedItem::ImageSnapshot>("ImageSnapshot");

    entt::meta_factory<StatisticalItem>()
        .type("StatisticalItem")
        .data<nullptr, &StatisticalItem::Sales>("Sales")
        .data<nullptr, &StatisticalItem::Favorites>("Favorites")
        .data<nullptr, &StatisticalItem::Likes>("Likes")
        .data<nullptr, &StatisticalItem::Dislikes>("Dislikes")
        .data<&StatisticalItem::Historical_Sales>("Historical_Sales")
        .data<&StatisticalItem::Historical_Favorites>("Historical_Favorites")
        .data<&StatisticalItem::Historical_Likes>("Historical_Likes")
        .data<&StatisticalItem::Historical_Dislikes>("Historical_Dislikes");

    entt::meta_factory<UniverseItem>()
        .type("UniverseItem")
        .data<&UniverseItem::UniverseId>("UniverseId");

    entt::meta_factory<Asset>()
        .type("Asset")
        .data<&Asset::Type>("Type")
        .data<&Asset::Public>("Public")

        .data<&Asset::MaxPlayers>("MaxPlayers")
        .data<&Asset::AllowDirectAccess>("AllowDirectAccess")
        .data<&Asset::Thumbnails>("Thumbnails")
        .data<&Asset::GearGenrePermission>("GearGenrePermission")
        .data<&Asset::GearTypes>("GearTypes")

        .data<&Asset::CurrencyType>("CurrencyType")
        .data<&Asset::Price>("Price")
        .data<&Asset::IsNew>("IsNew")
        .data<&Asset::LimitedType>("LimitedType")
        .data<&Asset::Remaining>("Remaining")
        .data<&Asset::ContentRatingTypeId>("ContentRatingTypeId")
        .data<&Asset::MinimumMembershipLevel>("MinimumMembershipLevel");
}