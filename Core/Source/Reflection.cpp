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

// do this as the reflection macros do not support scope operators
using AssetType = Roblox::AssetType;

/*
NOOBWARRIOR_REFLECT_ITEMTYPE_BEGIN(Asset)
    NOOBWARRIOR_REFLECT_DEFAULT_IMAGE(const_cast<int*>(g_question_mark_png), g_question_mark_png_size)
    NOOBWARRIOR_REFLECT_PROPERTY(
        Id, // Field Name
        "Id", // Pretty Name
        "The ID of this asset." // Field Description
    )

    NOOBWARRIOR_REFLECT_PROPERTY(
        Snapshot, // Field Name
        "Snapshot", // Pretty Name
        "The snapshot number." // Field Description
    )

    NOOBWARRIOR_REFLECT_PROPERTY(
        Name, // Field Name
        "Name", // Pretty Name
        "The name of this asset. Self-explanatory." // Field Description
    )

    NOOBWARRIOR_REFLECT_PROPERTY(
        Description, // Field Name
        "Description", // Pretty Name
        "A few sentences that describe what this asset does." // Field Description
    )

    NOOBWARRIOR_REFLECT_PROPERTY(
        ImageId, // Field Name
        "Image ID", // Pretty Name
        "The ID of the image that this asset will display. Does not apply for asset types that use auto-generated thumbnails." // Field Description
    )

    NOOBWARRIOR_REFLECT_PROPERTY(
        ImageSnapshot, // Field Name
        "Image Snapshot", // Pretty Name
        "The snapshot number of the image ID. Does not apply for asset types that use auto-generated thumbnails." // Field Description
    )
NOOBWARRIOR_REFLECT_ITEMTYPE_END(Asset)

NOOBWARRIOR_REFLECT_ITEMTYPE_BEGIN(Badge)
    NOOBWARRIOR_REFLECT_FIELD(
        Description,
        Badge,
        "Description",
        std::string,
        "A few sentences that describe what this badge does.",
        [](Database *db) -> SqlValue {
            return "";
        }
    )

    NOOBWARRIOR_REFLECT_FIELD(
        ImageId,
        Badge,
        "Image ID",
        int64_t,
        "The ID of the image that this badge displays.",
        [](Database *db) -> SqlValue {
            return {}; // initialize as empty object to return null
        }
    )

    NOOBWARRIOR_REFLECT_FIELD(
        ImageSnapshot,
        Badge,
        "Image Snapshot",
        int,
        "The snapshot number of the image ID.",
        [](Database *db) -> SqlValue {
            return {};
        }
    )
NOOBWARRIOR_REFLECT_ITEMTYPE_END(Badge)

NOOBWARRIOR_REFLECT_ITEMTYPE_BEGIN(Bundle)
NOOBWARRIOR_REFLECT_ITEMTYPE_END(Bundle)

NOOBWARRIOR_REFLECT_ITEMTYPE_BEGIN(DevProduct)
NOOBWARRIOR_REFLECT_ITEMTYPE_END(DevProduct)

NOOBWARRIOR_REFLECT_ITEMTYPE_BEGIN(Group)
NOOBWARRIOR_REFLECT_ITEMTYPE_END(Group)

NOOBWARRIOR_REFLECT_ITEMTYPE_BEGIN(Pass)
NOOBWARRIOR_REFLECT_ITEMTYPE_END(Pass)

NOOBWARRIOR_REFLECT_ITEMTYPE_BEGIN(Set)
NOOBWARRIOR_REFLECT_ITEMTYPE_END(Set)

NOOBWARRIOR_REFLECT_ITEMTYPE_BEGIN(Universe)
NOOBWARRIOR_REFLECT_ITEMTYPE_END(Universe)

NOOBWARRIOR_REFLECT_ITEMTYPE_BEGIN(User)
NOOBWARRIOR_REFLECT_ITEMTYPE_END(User)
*/

/*
// start creating common fields in all of them now because we now have the list of reflected id types
NOOBWARRIOR_REFLECT_COMMON_BEGIN
    NOOBWARRIOR_REFLECT_COMMON_FIELD(
        Name, // Field Name
        "Name",
        std::string, // Field Datatype
        "The name of this item. Self-explanatory.", // Field Description
        // Default Value
        [](Database *db) -> std::any {
            return "";
        },
        // Getter
        [](Database *db, int64_t id, std::optional<int> snapshot) -> std::any {
            return std::any(db->GetIdRowColumn<std::string>("Asset", id, snapshot, "Name"));
        },
        // Setter
        [](Database *db, int64_t id, std::optional<int> snapshot, std::any val) -> DatabaseResponse {
            return db->UpdateIdRowColumn<std::string>("Asset", id, snapshot, "Name", std::any_cast<std::string>(val));
        }
    )
NOOBWARRIOR_REFLECT_COMMON_END
*/

/*
NOOBWARRIOR_REFLECT_ENUM_BEGIN(AssetType)
    NOOBWARRIOR_REFLECT_ENUM_VALUE(None, AssetType::None)
    NOOBWARRIOR_REFLECT_ENUM_VALUE(Image, AssetType::Image)
    NOOBWARRIOR_REFLECT_ENUM_VALUE(TShirt, AssetType::TShirt)
    NOOBWARRIOR_REFLECT_ENUM_VALUE(Audio, AssetType::Audio)
NOOBWARRIOR_REFLECT_ENUM_END(AssetType)
*/

void NoobWarrior::Reflection::hi() {
    // put this here so that this source file can be referenced outside and therefore the compiler will be forced to link it and it wont be discarded
    // ps: this function is called in the constructor in Core.cpp
}

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