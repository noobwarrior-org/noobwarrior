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

using namespace NoobWarrior;

// do this as the reflection macros do not support scope operators
using AssetType = Roblox::AssetType;

NOOBWARRIOR_REFLECT_ITEMTYPE_BEGIN(Asset)
    NOOBWARRIOR_REFLECT_DEFAULT_IMAGE(const_cast<int*>(g_question_mark_png), g_question_mark_png_size)
    NOOBWARRIOR_REFLECT_FIELD(
        Id, // Field Name
        Asset, // Table Name
        "Id", // Pretty Name
        int64_t, // Field Datatype
        "The ID of this asset.", // Field Description
        // Default Value
        [](Database *db) -> SqlValue {
            db->ExecStatement("SELECT MAX(rowid) FROM Asset");
            return "";
        }
    )

    NOOBWARRIOR_REFLECT_FIELD(
        Snapshot, // Field Name
        Asset, // Table Name
        "Snapshot", // Pretty Name
        int, // Field Datatype
        "The snapshot number.", // Field Description
        // Default Value
        [](Database *db) -> SqlValue {
            db->ExecStatement("SELECT MAX(rowid) FROM Asset");
            return "";
        }
    )

    NOOBWARRIOR_REFLECT_FIELD(
        Name, // Field Name
        Asset,
        "Name",
        std::string, // Field Datatype
        "The name of this asset. Self-explanatory.", // Field Description
        // Default Value
        [](Database *db) -> SqlValue {
            return "";
        }
    )

    NOOBWARRIOR_REFLECT_FIELD(
        Description,
        Asset,
        "Description",
        std::string,
        "A few sentences that describe what this asset does.",
        [](Database *db) -> SqlValue {
            return "";
        }
    )

    NOOBWARRIOR_REFLECT_FIELD(
        ImageId,
        Asset,
        "Image ID",
        int64_t,
        "The ID of the image that this asset will display. Does not apply for asset types that use auto-generated thumbnails.",
        [](Database *db) -> SqlValue {
            return {}; // initialize as empty object to return null
        }
    )

    NOOBWARRIOR_REFLECT_FIELD(
        ImageSnapshot,
        Asset,
        "Image Snapshot",
        int,
        "The snapshot number of the image ID. Does not apply for asset types that use auto-generated thumbnails.",
        [](Database *db) -> SqlValue {
            return {};
        }
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