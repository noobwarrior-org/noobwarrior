// === noobWarrior ===
// File: Archive.h
// Started by: Hattozo
// Started on: 2/17/2025
// Description: Encapsulates a SQLite database and creates tables containing Roblox assets and other kinds of data
#pragma once
#include <NoobWarrior/Roblox/Api/Asset.h>
#include <NoobWarrior/Roblox/IdType.h>

#include <sqlite3.h>

#include <filesystem>
#include <string>
#include <vector>
#include <cstdint>

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
        /**
         * @return Returns the file name of the Archive's currently loaded file.
           If a file is currently not loaded or if the archive is stored in memory only it returns a blank string.
           This does not return a file path, do not confuse this function with returning one.
         */
        std::string GetFileName();

        std::filesystem::path GetFilePath();

        int AddAsset(Roblox::AssetDetails *asset);
        std::vector<unsigned char> RetrieveFile(int64_t id, Roblox::IdType type);
    private:
        std::filesystem::path mPath;
        sqlite3 *mDatabase;
        bool mInitialized;
    };
}