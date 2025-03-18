// === noobWarrior ===
// File: Archive.h
// Started by: Hattozo
// Started on: 2/17/2025
// Description: Encapsulates a SQLite database and creates tables containing Roblox assets and other kinds of data
#pragma once
#include <NoobWarrior/Roblox/Api/Asset.h>

#include <sqlite3.h>

#include <string>

#define NOOBWARRIOR_ARCHIVE_VERSION 1

namespace NoobWarrior {
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

        int AddAsset(Roblox::AssetDetails *asset);
    private:
        std::string mPath;
        sqlite3 *mDatabase;
    };
}