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
// File: EmuDb.h
// Started by: Hattozo
// Started on: 2/17/2025
// Description: A high-level interface to access a specialized noobWarrior SQLite database that contains Roblox items and other data
/*
 * EmuDb File Format Documentation
 * ====== Purpose ======
 * The purpose of the EmuDb file format is to store items in a format
 * that is compatible with the Roblox web backend and game engine.
 * Additionally, it is also repurposed to be an all-in-one place for the
 * developer to store their works for a project.
 *
 * ====== Technical Overview ======
 * The EmuDb file format uses SQLite for its backend. All read/write
 * operations to the file are handled with SQL.
 *
 * Changes to the file format are made through constructing new SQL statements
 * in "migrations" and applying them in order when the file is opened.
 *
 * Its official file extension is ".nwdb" and it is intended to be used in
 * the noobWarrior software.
 *
 * Officially, developers should interface with the file using either the C++,
 * C, or Lua API's that we have created. Do not manually execute SQL statements
 * into the database if you want to guarantee stability.
 *
 * From the C++ abstraction side, all Roblox items are accessed through
 * repositories
 *
 * ====== Key SQL Tables ======
 * === Meta ===
 * This is where metadata about the database is stored.
 * It contains a few properties that can be changed by the user.
 * == Properties ===
 * (Title) This is, of course, the title.
 * (Description) What describes this database?
 * (Version) The version of the database. Please note that this is not the
 * version of the file format, but a set value by the author in order to
 * denote the version of the users project.
 * (Icon) A Base64 encoded string that contains a valid image file.
 * (Mutable) Allows the database to be modified by players during runtime.
 * (CompressionType) A boolean that corresponds to CompressionType enum.
 * If set with API, it will compress all binary blobs in the database using the
 * specified compression algorithm.
 * (OnlyEnableIfServerWithPlaceFromThisDatabaseIsRunning) Items from this
 * database will only be requested by the emulator if one of your running game
 * servers has loaded a place from this database.
 * (TakeHigherPriorityIfServerWithPlaceFromThisDatabaseIsRunning) Makes the
 * database have a higher priority if one of the running game servers has
 * loaded a place from this database. You can turn this on if you are
 * paranoid of conflicting ID's. WARNING: This prevents people from being able
 * to make asset replacement mods for your game.
 */

#pragma once
#include <NoobWarrior/SqlDb/SqlDb.h>
#include <NoobWarrior/SqlDb/Common.h>
#include <NoobWarrior/SqlDb/Statement.h>
#include <NoobWarrior/EmuDb/ContentImages.h>
#include <NoobWarrior/EmuDb/Repository/Item/AssetRepository.h>
#include <NoobWarrior/EmuDb/Item/Asset.h>
#include <NoobWarrior/EmuDb/Item/Universe.h>
#include <NoobWarrior/EmuDb/Item/User.h>
#include <NoobWarrior/EmuDb/Item/Badge.h>

#include <NoobWarrior/Roblox/Api/Asset.h>
#include <NoobWarrior/Log.h>

#include <sqlite3.h>

#include <filesystem>
#include <string>
#include <vector>
#include <cstdint>

#define NOOBWARRIOR_BEGIN_PROP_SETTER for (int i = 0; i < sqlite3_column_count(stmt); i++) {
#define NOOBWARRIOR_END_PROP_SETTER }
#define NOOBWARRIOR_SET_PROP_FIELD(field, tableFieldName, type) if (strncmp(sqlite3_column_name(stmt, i), tableFieldName, 4) == 0) { \
	auto var = sqlite3_column_##type(stmt, i); \
	field = var; \
    }

#define NOOBWARRIOR_BIND_FIELD()

namespace NoobWarrior {
struct SearchOptions {
    int Offset{0}; // Where do you want to start?
    int Limit{99}; // How much do you want to go up to?
    std::string Query{}; // Leave blank to make it search the entire database
    std::optional<int64_t> CreatorId{}; // Leave null to disable creator filter
    Roblox::CreatorType CreatorType{};
    Roblox::AssetType AssetType{Roblox::AssetType::None}; // Does not apply if you aren't searching for assets
};

/**
 * @brief A noobWarrior database that can contain Roblox assets, users, games, etc.
 */
class EmuDb : public SqlDb {
    friend class Statement;
public:
    enum class CompressionType {
        None,
        ZStandard
    };

    /**
     * @param autocommit Will enable SQLite's auto-commit feature if true; any writes you do to the database are immediately saved to disk. Set this to false if you are not using this in the context of a rapidly changing online database.
     */
    EmuDb(const std::string &path = ":memory:", bool autocommit = true);

    int GetMigrationVersion();

    SqlDb::Response SaveAs(const std::string &path);

    /**
     * @brief Commits the current SQLite transaction, which will write all changes to disk.
     */
    SqlDb::Response WriteChangesToDisk();

    /**
     * @brief Returns true if this database has unsaved changes.
     */
    bool IsDirty();
    void MarkDirty();
    void UnmarkDirty();

    std::string GetMigrationFailMsg();

    std::string GetMetaKeyValue(const std::string &key);
    std::string GetTitle();
    std::string GetDescription();
    std::string GetVersion();
    std::string GetAuthor();
    std::vector<unsigned char> GetIcon();

    SqlDb::Response SetMetaKeyValue(const std::string &key, const std::string &value);
    SqlDb::Response SetTitle(const std::string &title);
    SqlDb::Response SetAuthor(const std::string &author);
    SqlDb::Response SetIcon(const std::vector<unsigned char> &icon);
    
    SqlDb::Response AddBlob(const std::vector<unsigned char> &data);

    AssetRepository* GetAssetRepository();

    std::vector<unsigned char> RetrieveImageData(const std::string &tableName, int id);

    template<typename T>
    static T GetValueFromColumnIndex(sqlite3_stmt *stmt, int columnIndex) {
        if constexpr (std::is_same_v<T, int> || std::is_same_v<T, bool>)
            return sqlite3_column_int(stmt, columnIndex);
        if constexpr (std::is_same_v<T, int64_t>)
            return sqlite3_column_int64(stmt, columnIndex);
        if constexpr (std::is_same_v<T, std::string>) {
            return std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, columnIndex)));
        }
        if constexpr (std::is_same_v<T, std::vector<unsigned char>>) {
            std::vector<unsigned char> data;
            auto *buf = static_cast<unsigned char*>(const_cast<void*>(sqlite3_column_blob(stmt, columnIndex)));
            data.assign(buf, buf + sqlite3_column_bytes(stmt, columnIndex));
            return data;
        }
        T def {};
        return def;
    }

    // TODO: Optimize this so that it caches the results instead of having to go through the for loop everytime.
    template<typename T>
    static T GetValueFromColumnName(sqlite3_stmt *stmt, const std::string &columnName) {
        for (int i = 0; i < sqlite3_column_count(stmt); i++) {
            if (strncmp(sqlite3_column_name(stmt, i), columnName.c_str(), strlen(columnName.c_str())) == 0) {
                return GetValueFromColumnIndex<T>(stmt, i);
            }
        }
        T def {};
        return def;
    }

    static inline int GetTypeFromColumnName(sqlite3_stmt *stmt, const std::string &columnName) {
        for (int i = 0; i < sqlite3_column_count(stmt); i++) {
            if (strncmp(sqlite3_column_name(stmt, i), columnName.c_str(), strlen(columnName.c_str())) == 0) {
                return sqlite3_column_type(stmt, i);
            }
        }
        return 0;
    }

    static inline int GetBlobSizeFromColumnName(sqlite3_stmt *stmt, const std::string &columnName) {
        for (int i = 0; i < sqlite3_column_count(stmt); i++) {
            if (strncmp(sqlite3_column_name(stmt, i), columnName.c_str(), strlen(columnName.c_str())) == 0) {
                return sqlite3_column_bytes(stmt, i);
            }
        }
        return 0;
    }
protected:
    inline sqlite3_stmt* ConstructIdRecordStmtFromName(const std::string name, const int64_t id, const std::optional<int> &snapshot) {
        std::string stmtStr = std::format("SELECT * FROM {} WHERE Id = ? {};", name, snapshot.has_value() ? "AND Snapshot = ?" : "ORDER BY Snapshot DESC LIMIT 1");

        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(mDb, stmtStr.c_str(), -1, &stmt, nullptr);
        sqlite3_bind_int64(stmt, 1, id);
        if (snapshot.has_value())
            sqlite3_bind_int(stmt, 2, snapshot.value());

        return stmt;
    }

    std::vector<unsigned char> RetrieveBlobFromTableName(int64_t id, const std::string &tableName, const std::string &columnName);

    AssetRepository mAssetRepository;
private:
    bool VerifyIntegrityOfMigration();
    bool MigrateToLatestVersion();

    std::filesystem::path mPath;
    bool mAutoCommit;
    bool mDirty;

    std::string mMigrationFailMsg;
};
}
