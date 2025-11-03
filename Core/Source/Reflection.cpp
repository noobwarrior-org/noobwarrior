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
#include <NoobWarrior/Reflection.h>

#include <NoobWarrior/Database/Database.h>
#include <NoobWarrior/Database/Record/IdType/Asset.h>
#include <NoobWarrior/Database/Record/IdType/Badge.h>
#include <NoobWarrior/Database/Record/IdType/Bundle.h>
#include <NoobWarrior/Database/Record/IdType/DevProduct.h>
#include <NoobWarrior/Database/Record/IdType/Group.h>
#include <NoobWarrior/Database/Record/IdType/Pass.h>
#include <NoobWarrior/Database/Record/IdType/Set.h>
#include <NoobWarrior/Database/Record/IdType/Universe.h>
#include <NoobWarrior/Database/Record/IdType/User.h>

using namespace NoobWarrior;

// do this as the reflection macros do not support scope operators
using AssetType = Roblox::AssetType;

NOOBWARRIOR_REFLECT_ID_TYPE_BEGIN(Asset)
    NOOBWARRIOR_REFLECT_FIELD(
        Name, // Field Name
        std::string, // Field Datatype
        "The name of this asset. Self-explanatory.", // Field Description
        // Default Value
        []() -> std::any {
            return "";
        },
        // Getter
        [](Database *db, int64_t id, std::optional<int> version) -> std::any {
            return std::any(db->GetIdRowColumn<std::string>("Asset", id, version, "Name"));
        },
        // Setter
        [](Database *db, int64_t id, std::optional<int> version, std::any val) -> DatabaseResponse {
            return db->UpdateIdRowColumn<std::string>("Asset", id, version, "Name", std::any_cast<std::string>(val));
        }
    )

    NOOBWARRIOR_REFLECT_FIELD(
        Description,
        std::string,
        "A few sentences that describe what this asset does.",
        []() -> std::any {
            return "";
        },
        [](Database *db, int64_t id, std::optional<int> version) -> std::any {
            return std::any(db->GetIdRowColumn<std::string>("Asset", id, version, "Description"));
        },
        [](Database *db, int64_t id, std::optional<int> version, std::any val) -> DatabaseResponse {
            return db->UpdateIdRowColumn<std::string>("Asset", id, version, "Description", std::any_cast<std::string>(val));
        }
    )

    NOOBWARRIOR_REFLECT_FIELD(
        ImageId,
        int64_t,
        "The ID of the image that this asset will display. Does not apply for asset types that use auto-generated thumbnails.",
        []() -> std::any {
            return {}; // initialize as empty object to return null
        },
        [](Database *db, int64_t id, std::optional<int> version) -> std::any {
            return std::any(db->GetIdRowColumn<int64_t>("Asset", id, version, "ImageId"));
        },
        [](Database *db, int64_t id, std::optional<int> version, std::any val) -> DatabaseResponse {
            return db->UpdateIdRowColumn<int64_t>("Asset", id, version, "ImageId", std::any_cast<int64_t>(val));
        }
    )

    NOOBWARRIOR_REFLECT_FIELD(
        ImageVersion,
        int,
        "The version of the image ID. Does not apply for asset types that use auto-generated thumbnails.",
        []() -> std::any {
            return {};
        },
        [](Database *db, int64_t id, std::optional<int> version) -> std::any {
            return std::any(db->GetIdRowColumn<int>("Asset", id, version, "ImageVersion"));
        },
        [](Database *db, int64_t id, std::optional<int> version, std::any val) -> DatabaseResponse {
            return db->UpdateIdRowColumn<int>("Asset", id, version, "ImageVersion", std::any_cast<int>(val));
        }
    )

    /*
    NOOBWARRIOR_REFLECT_FIELD(CreatorId, User, "The ID of the entity that created this asset.", [](Database *db, int64_t id, int64_t version) -> std::any {
        return db->GetIdRowColumn("Asset", id, version, "Description");
    }, [](Database *db, int64_t id, int64_t version, std::any val) -> void {
        db->UpdateIdRowColumn("Asset", id, version, "Description", val);
    })

    NOOBWARRIOR_REFLECT_FIELD(CreatorType, User, "Is this asset owned by an individual user or a group?", [](Database *db, int64_t id, int64_t version) -> std::any {
        return db->GetIdRowColumn("Asset", id, version, "Description");
    }, [](Database *db, int64_t id, int64_t version, std::any val) -> void {
        db->UpdateIdRowColumn("Asset", id, version, "Description", val);
    })
    */
NOOBWARRIOR_REFLECT_ID_TYPE_END(Asset)

NOOBWARRIOR_REFLECT_ID_TYPE_BEGIN(Badge)
NOOBWARRIOR_REFLECT_ID_TYPE_END(Badge)

NOOBWARRIOR_REFLECT_ID_TYPE_BEGIN(Bundle)
NOOBWARRIOR_REFLECT_ID_TYPE_END(Bundle)

NOOBWARRIOR_REFLECT_ID_TYPE_BEGIN(DevProduct)
NOOBWARRIOR_REFLECT_ID_TYPE_END(DevProduct)

NOOBWARRIOR_REFLECT_ID_TYPE_BEGIN(Group)
NOOBWARRIOR_REFLECT_ID_TYPE_END(Group)

NOOBWARRIOR_REFLECT_ID_TYPE_BEGIN(Pass)
NOOBWARRIOR_REFLECT_ID_TYPE_END(Pass)

NOOBWARRIOR_REFLECT_ID_TYPE_BEGIN(Set)
NOOBWARRIOR_REFLECT_ID_TYPE_END(Set)

NOOBWARRIOR_REFLECT_ID_TYPE_BEGIN(Universe)
NOOBWARRIOR_REFLECT_ID_TYPE_END(Universe)

NOOBWARRIOR_REFLECT_ID_TYPE_BEGIN(User)
NOOBWARRIOR_REFLECT_ID_TYPE_END(User)

/*
// start creating common fields in all of them now because we now have the list of reflected id types
NOOBWARRIOR_REFLECT_COMMON_BEGIN
    NOOBWARRIOR_REFLECT_COMMON_FIELD(Name, std::string, "The name of this item. Self-explanatory.", [](Database *db) -> std::any {

    }, [](Database *db, std::any) -> void {

    })
NOOBWARRIOR_REFLECT_COMMON_END
*/

NOOBWARRIOR_REFLECT_ENUM_BEGIN(AssetType)
    NOOBWARRIOR_REFLECT_ENUM_VALUE(None, AssetType::None)
    NOOBWARRIOR_REFLECT_ENUM_VALUE(Image, AssetType::Image)
    NOOBWARRIOR_REFLECT_ENUM_VALUE(TShirt, AssetType::TShirt)
    NOOBWARRIOR_REFLECT_ENUM_VALUE(Audio, AssetType::Audio)
NOOBWARRIOR_REFLECT_ENUM_END(AssetType)

void NoobWarrior::Reflection::hi() {
    // put this here so that this source file can be referenced outside and therefore the compiler will be forced to link it and it wont be discarded
    // ps: this function is called in the constructor in Core.cpp
}