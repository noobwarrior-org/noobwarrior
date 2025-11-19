// === noobWarrior ===
// File: Database.h
// Started by: Hattozo
// Started on: 2/17/2025
// Description: A high-level interface to access a specialized noobWarrior SQLite database that contains Roblox items and other data
#pragma once
#include <NoobWarrior/Database/Common.h>
#include <NoobWarrior/Database/Statement.h>
#include <NoobWarrior/Database/ContentImages.h>
#include <NoobWarrior/Database/Repository/Item/AssetRepository.h>
#include <NoobWarrior/Database/Item/Asset.h>
#include <NoobWarrior/Database/Item/Universe.h>
#include <NoobWarrior/Database/Item/User.h>
#include <NoobWarrior/Database/Item/Badge.h>
#include <NoobWarrior/Reflection.h>

#include <NoobWarrior/Roblox/Api/Asset.h>
#include <NoobWarrior/Log.h>

#include <sqlite3.h>

#include <filesystem>
#include <string>
#include <vector>
#include <cstdint>

#define NOOBWARRIOR_DATABASE_VERSION 1

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
class Database {
    friend class Statement;
public:
    enum class CompressionType {
        None,
        ZStandard
    };

    /**
     * @param autocommit Will enable SQLite's auto-commit feature if true; any writes you do to the database are immediately saved to disk. Set this to false if you are not using this in the context of a rapidly changing online database.
     */
    Database(bool autocommit = true);

    /**
     * @brief Initializes the database and makes it ready to perform read/write operations to it.
     * @param path What is the file path of the database you want to open? If none is given, it will just create one in memory.
     * @return An enum that tells you if it was successful or not.
     */
    DatabaseResponse Open(const std::string &path = ":memory:");

    int Close();

    int GetDatabaseVersion();

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

    void UnmarkDirty();

    /**
     * @brief Returns true if this database is not yet a tangible file and only exists within memory.
     */
    bool IsMemory();

    bool DoesItemExist(const Reflection::IdType &idType, int64_t id, std::optional<int> snapshot = std::nullopt);

    std::string GetSqliteErrorMsg();
    std::string GetMetaKeyValue(const std::string &key);

    /**
     * @return Returns the file name of the database's currently loaded file.
     If a file is currently not loaded or if the database is stored in memory only it returns a blank string.
     This does not return a file path, do not confuse this function with returning one.
     */
    std::string GetFileName();
    std::filesystem::path GetFilePath();

    std::string GetTitle();
    std::string GetDescription();
    std::string GetVersion();
    std::string GetAuthor();
    std::vector<unsigned char> GetIcon();

    DatabaseResponse ExecSqlStatement(const std::string &stmtStr);
    DatabaseResponse SetMetaKeyValue(const std::string &key, const std::string &value);
    DatabaseResponse SetTitle(const std::string &title);
    DatabaseResponse SetAuthor(const std::string &author);
    DatabaseResponse SetIcon(const std::vector<unsigned char> &icon);

    AssetRepository& GetAssetRepository();

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
    int SetDatabaseVersion(int version);

    inline sqlite3_stmt* ConstructIdRecordStmtFromName(const std::string name, const int64_t id, const std::optional<int> &snapshot) {
        std::string stmtStr = std::format("SELECT * FROM {} WHERE Id = ? {};", name, snapshot.has_value() ? "AND Snapshot = ?" : "ORDER BY Snapshot DESC LIMIT 1");

        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(mDatabase, stmtStr.c_str(), -1, &stmt, nullptr);
        sqlite3_bind_int64(stmt, 1, id);
        if (snapshot.has_value())
            sqlite3_bind_int(stmt, 2, snapshot.value());

        return stmt;
    }

    std::vector<unsigned char> RetrieveBlobFromTableName(int64_t id, const std::string &tableName, const std::string &columnName);

    AssetRepository mAssetRepository;
private:
    std::filesystem::path mPath;
    sqlite3 *mDatabase;
    bool mInitialized;
    bool mAutoCommit;
    bool mDirty;
};
}
