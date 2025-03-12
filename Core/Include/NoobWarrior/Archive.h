// === noobWarrior ===
// File: Archive.h
// Started by: Hattozo
// Started on: 2/17/2025
// Description: Encapsulates a SQLite database and creates tables containing Roblox assets and other kinds of data
#pragma once

#include <sqlite3.h>

#include <string>
#include <cstdint>

namespace NoobWarrior {
    enum class AssetType { Image = 1, TShirt = 2, Audio = 3, Mesh = 4, Lua = 5,
                           Hat = 8, Place = 9, Model = 10, Shirt = 11, Pants = 12,
                           Decal = 13, Head = 17, Face = 18, Gear = 19, Badge = 21,
                           Animation = 24, Torso = 27, RightArm = 28, LeftArm = 29,
                           LeftLeg = 30, RightLeg = 31, Package = 32, Gamepass = 34,
                           Plugin = 38, MeshPart = 40, HairAccessory = 41, FaceAccessory = 42,
                           NeckAccessory = 43, ShoulderAccessory = 44, FrontAccessory = 45,
                           BackAccessory = 46, WaistAccessory = 47, ClimbAnimation = 48,
                           DeathAnimation = 49, FallAnimation = 50, IdleAnimation = 51,
                           JumpAnimation = 52, RunAnimation = 53, SwimAnimation = 54,
                           WalkAnimation = 55, PoseAnimation = 56, EarAccessory = 57,
                           EyeAccessory = 58, EmoteAnimation = 61, Video = 62, TShirtAccessory = 64,
                           ShirtAccessory = 65, PantsAccessory = 66, JacketAccessory = 67,
                           SweaterAccessory = 68, ShortsAccessory = 69, LeftShoeAccessory = 70,
                           RightShoeAccessory = 71, DressSkirtAccessory = 72, FontFamily = 73,
                           EyebrowAccessory = 76, EyelashAccessory = 77, MoodAnimation = 78,
                           DynamicHead = 79 };

    enum class CreatorType { User, Group };

    typedef struct {
        int             Id;
        const char*     Name;
        const char*     Description;
        uint64_t        Created;
        uint64_t        Updated;
        int             Type;
        CreatorType     CreatorType;
        int64_t         CreatorId;
        int             PriceInRobux;
        int             ContentRatingTypeId;
        int             MinimumMembershipLevel;
        uint8_t         IsPublicDomain;
        uint8_t         IsForSale;
        uint8_t         IsLimitedUnique;
        uint8_t         IsNew;
        unsigned int    Remaining;
        unsigned int    Sales;
        void*           Data;
    } Asset;

    class Archive {
    public:
        Archive();

        int Open(const std::string &path = ":memory:");
        int Close();
        int SaveAs(const std::string &path);
        int GetDatabaseVersion();
        int SetDatabaseVersion(int version);

        /**
         * @brief Commits the current SQLite transaction, which will write all changes to disk.
         */
        int WriteChangesToDisk();

        /**
         * @brief Returns true if this archive has unsaved changes.
         */
        bool IsDirty();
        std::string GetSqliteErrorMsg();
        std::string GetTitle();

        int AddAsset(Asset *asset);
    private:
        std::string mPath;
        sqlite3 *mDatabase;
    };
}