// === noobWarrior ===
// File: Database.h
// Started by: Hattozo
// Started on: 2/17/2025
// Description: Encapsulates a SQLite database and creates tables containing Roblox assets and other kinds of data
#pragma once
#include "Asset.h"
#include "../Roblox/Api/Asset.h"
#include "../Roblox/IdType.h"

#include <sqlite3.h>

#include <filesystem>
#include <string>
#include <vector>
#include <cstdint>

#define NOOBWARRIOR_DATABASE_VERSION 1

namespace NoobWarrior {
    struct SearchOptions {
        int Offset { 0 }; // Where do you want to start?
        int Limit { 99 }; // How much do you want to go up to?

        std::string Query;
    };

    enum class DatabaseResponse {
        Failed,
        Success,
        CouldNotOpen,
        CouldNotGetVersion,
        CouldNotSetVersion,
        CouldNotCreateTable,
        CouldNotSetKeyValues,
        DidNothing,
        NotInitialized
    };

    class Database {
    public:
        Database(bool autocommit = true);

        DatabaseResponse Open(const std::string &path = ":memory:");
        int Close();
        int GetDatabaseVersion();
        int SetDatabaseVersion(int version);

        DatabaseResponse SaveAs(const std::string &path);

        /**
         * @brief Commits the current SQLite transaction, which will write all changes to disk.
         */
        DatabaseResponse WriteChangesToDisk();

        /**
         * @brief Returns true if this database has unsaved changes.
         */
        bool IsDirty();
        void MarkDirty();

        std::string GetSqliteErrorMsg();
        std::string GetTitle();
        /**
         * @return Returns the file name of the database's currently loaded file.
           If a file is currently not loaded or if the database is stored in memory only it returns a blank string.
           This does not return a file path, do not confuse this function with returning one.
         */
        std::string GetFileName();

        std::filesystem::path GetFilePath();

        DatabaseResponse AddAsset(Asset *asset);
        std::vector<unsigned char> RetrieveContent(int64_t id, Roblox::IdType type);

        std::vector<Asset> GetAssets(const SearchOptions &opt);
    private:
        std::filesystem::path mPath;
        sqlite3 *mDatabase;
        bool mInitialized;
        bool mAutoCommit;
        bool mDirty;
    };
}