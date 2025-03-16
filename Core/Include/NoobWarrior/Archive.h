// === noobWarrior ===
// File: Archive.h
// Started by: Hattozo
// Started on: 2/17/2025
// Description: Encapsulates a SQLite database and creates tables containing Roblox assets and other kinds of data
#pragma once

#include <sqlite3.h>

#include <string>
#include <cstdint>

#define NOOBWARRIOR_ARCHIVE_VERSION 1

namespace NoobWarrior {
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