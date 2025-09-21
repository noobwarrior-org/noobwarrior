// === noobWarrior ===
// File: Database.h
// Started by: Hattozo
// Started on: 2/17/2025
// Description: Encapsulates a SQLite database and creates tables containing Roblox assets and other kinds of data
#pragma once
#include <NoobWarrior/ReflectionMetadata.h>
#include "Record/IdType/Asset.h"
#include "Record/IdType/Badge.h"
#include "Record/IdType/Universe.h"
#include "Record/IdType/User.h"
#include "ContentImages.h"

#include "../Roblox/Api/Asset.h"
#include "../Log.h"

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

    enum class DatabaseResponse {
        Failed,
        Success,
        CouldNotOpen,
        CouldNotGetVersion,
        CouldNotSetVersion,
        CouldNotCreateTable,
        CouldNotSetKeyValues,
        DidNothing,
        NotInitialized,
        StatementConstraintViolation,
        Busy,
        Misuse
    };

    /**
     * @brief A noobWarrior database that can contain Roblox assets, users, games, etc.
     */
    class Database {
        friend class Statement;
    public:
        /**
         * @param autocommit Will enable SQLite's auto-commit feature if true; any writes you do to the database are immediately saved to disk.
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

        DatabaseResponse SetMetaKeyValue(const std::string &key, const std::string &value);
        DatabaseResponse SetTitle(const std::string &title);
        DatabaseResponse SetAuthor(const std::string &author);
        DatabaseResponse SetIcon(const std::vector<unsigned char> &icon);

        /**
         * @brief Gets the size of the asset's data in bytes
         */
        int GetAssetSize(int64_t id);

        /**
         * @param id The asset ID
         * @return The raw data that the asset contains
         */
        std::vector<unsigned char> RetrieveAssetData(int64_t id);

        template<typename T>
        std::vector<unsigned char> RetrieveContentBlob(int64_t id, const std::string &columnName) {
            static_assert(std::is_base_of_v<IdRecord, T>, "typename must inherit from IdRecord");
            return RetrieveBlobFromTableName(id, Reflection::GetIdTypeName<T>(), columnName);
        }

        template<typename T>
        std::vector<unsigned char> RetrieveContentImageData(int64_t id) {
            static_assert(std::is_base_of_v<IdRecord, T>, "typename must inherit from IdRecord");
            if (!mInitialized) return {};

            std::string stmtStr = std::format("SELECT * FROM {} WHERE Id = ? ORDER BY Version DESC LIMIT 1;",
                                              Reflection::GetIdTypeName<T>());

            sqlite3_stmt *stmt;
            sqlite3_prepare_v2(mDatabase, stmtStr.c_str(), -1, &stmt, nullptr);
            sqlite3_bind_int64(stmt, 1, id);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                const auto iconId = GetValueFromColumnName<int64_t>(stmt, "Image");
                if (std::vector<unsigned char> imageData = RetrieveAssetData(iconId); !imageData.empty()) {
                    sqlite3_finalize(stmt);
                    return imageData;
                }
            }
            sqlite3_finalize(stmt);

            std::vector<unsigned char> data;

            if (std::is_same_v<T, Asset>) {
                std::optional<Asset> asset = GetContent<Asset>(id);
                if (asset.has_value()) {
                    data = GetImageForAssetType(asset.value().Type);
                }
            }

            if (data.empty()) data.assign(T::DefaultImage, T::DefaultImage + T::DefaultImageSize);
            return data;
        }

        template<typename T>
        std::optional<T> GetContent(const int64_t id, const std::optional<int> version = std::nullopt) {
            static_assert(std::is_base_of_v<IdRecord, T>, "typename must inherit from IdRecord");
            if (!mInitialized) return std::nullopt;

            std::string stmtStr = std::format("SELECT * FROM {} WHERE Id = ? {};", Reflection::GetIdTypeName<T>(), version.has_value() ? "AND Version = ?" : "ORDER BY Version DESC LIMIT 1");

            sqlite3_stmt *stmt;
            sqlite3_prepare_v2(mDatabase, stmtStr.c_str(), -1, &stmt, nullptr);
            sqlite3_bind_int64(stmt, 1, id);
            if (version.has_value())
                sqlite3_bind_int(stmt, 2, version.value());

            while (sqlite3_step(stmt) == SQLITE_ROW) {
                T content {};
                content.Id = GetValueFromColumnName<int64_t>(stmt, "Id");
                content.Version = GetValueFromColumnName<int>(stmt, "Version");
                content.FirstRecorded = GetValueFromColumnName<int64_t>(stmt, "FirstRecorded");
                content.LastRecorded = GetValueFromColumnName<int64_t>(stmt, "LastRecorded");
                content.Name = GetValueFromColumnName<std::string>(stmt, "Name");

                if constexpr (std::is_base_of_v<OwnedIdRecord, T>) {
                    content.Description = GetValueFromColumnName<std::string>(stmt, "Description");
                    content.Created = GetValueFromColumnName<int64_t>(stmt, "Created");
                    content.Updated = GetValueFromColumnName<int64_t>(stmt, "Updated");
                }

                if constexpr (std::is_same_v<T, Asset>) {
                    content.Type = static_cast<Roblox::AssetType>(GetValueFromColumnName<int>(stmt, "Type"));
                    content.ImageId = GetValueFromColumnName<int64_t>(stmt, "ImageId");
                    content.ImageVersion = GetValueFromColumnName<int64_t>(stmt, "ImageVersion");
                    {
                        // The database table has separate fields for the creator ID, "UserId" and "GroupId", so that referencing foreign keys can be possible.
                        content.CreatorType = GetTypeFromColumnName(stmt, "UserId") != SQLITE_NULL
                                                  ? Roblox::CreatorType::User
                                                  : Roblox::CreatorType::Group;
                        content.CreatorId = GetTypeFromColumnName(stmt, "UserId") != SQLITE_NULL
                                                ? GetValueFromColumnName<int64_t>(stmt, "UserId")
                                                : GetValueFromColumnName<int64_t>(stmt, "GroupId");
                    }
                    content.CurrencyType = static_cast<Roblox::CurrencyType>(GetValueFromColumnName<int>(stmt, "CurrencyType"));
                    content.Price = GetValueFromColumnName<int>(stmt, "Price");
                    content.ContentRatingTypeId = GetValueFromColumnName<int>(stmt, "ContentRatingTypeId");
                    content.MinimumMembershipLevel = GetValueFromColumnName<int>(stmt, "MinimumMembershipLevel");
                    content.IsPublicDomain = GetValueFromColumnName<bool>(stmt, "IsPublicDomain");
                    content.IsForSale = GetValueFromColumnName<bool>(stmt, "IsForSale");
                    content.IsNew = GetValueFromColumnName<bool>(stmt, "IsNew");
                    content.LimitedType = static_cast<Roblox::LimitedType>(GetValueFromColumnName<int>(stmt, "LimitedType"));
                    content.Remaining = GetValueFromColumnName<int64_t>(stmt, "Remaining");

                    // Historical data
                    content.Sales = GetValueFromColumnName<int64_t>(stmt, "Historical_Sales");
                    content.Favorites = GetValueFromColumnName<int64_t>(stmt, "Historical_Favorites");
                    content.Likes = GetValueFromColumnName<int64_t>(stmt, "Historical_Likes");
                    content.Dislikes = GetValueFromColumnName<int64_t>(stmt, "Historical_Dislikes");

                    sqlite3_finalize(stmt);
                    return content;
                } else if constexpr (std::is_same_v<T, Badge>) {
                    sqlite3_finalize(stmt);
                    return content;
                }
            }

            sqlite3_finalize(stmt);
            return std::nullopt;
        }

        template<typename T>
        DatabaseResponse AddContent(const T &content, bool overwrite = false) {
            static_assert(std::is_base_of_v<IdRecord, T>, "typename must inherit from IdRecord");

            if (!mInitialized) return DatabaseResponse::NotInitialized;
            DatabaseResponse res = DatabaseResponse::Failed;
            sqlite3_stmt *stmt;

            std::string keyword = overwrite ? "REPLACE" : "INSERT";

            if constexpr (std::is_same_v<T, Asset>) {
                std::string stmtStr = std::format("{} INTO Asset (Id, Version, Name, Description, Created, Updated, Type, Image, UserId, GroupId, CurrencyType, Price, ContentRatingTypeId, MinimumMembershipLevel, IsPublicDomain, IsForSale, IsNew, LimitedType, Remaining, Historical_Sales, Historical_Favorites, Historical_Likes, Historical_Dislikes) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);", keyword);
                sqlite3_prepare_v2(
                    mDatabase,
                    stmtStr.c_str(),
                    -1, &stmt, nullptr);
                sqlite3_bind_int64(stmt, 1, content.Id);
                sqlite3_bind_int(stmt, 2, content.Version);
                sqlite3_bind_text(stmt, 3, content.Name.c_str(), -1, nullptr);
                sqlite3_bind_text(stmt, 4, content.Description.c_str(), -1, nullptr);
                sqlite3_bind_int64(stmt, 5, content.Created);
                sqlite3_bind_int64(stmt, 6, content.Updated);
                sqlite3_bind_int(stmt, 7, static_cast<int>(content.Type));
                sqlite3_bind_int64(stmt, 8, content.ImageId);
                sqlite3_bind_int64(stmt, 9, content.ImageVersion);
                content.CreatorType == Roblox::CreatorType::User
                    ? sqlite3_bind_int64(stmt, 10, content.CreatorId)
                    : sqlite3_bind_null(stmt, 10);
                content.CreatorType == Roblox::CreatorType::Group
                    ? sqlite3_bind_int64(stmt, 11, content.CreatorId)
                    : sqlite3_bind_null(stmt, 11);
                sqlite3_bind_int(stmt, 12, static_cast<int>(content.CurrencyType));
                sqlite3_bind_int(stmt, 13, content.Price);
                sqlite3_bind_int(stmt, 14, content.ContentRatingTypeId);
                sqlite3_bind_int(stmt, 15, content.MinimumMembershipLevel);
                sqlite3_bind_int(stmt, 16, content.IsPublicDomain);
                sqlite3_bind_int(stmt, 17, content.IsForSale);
                sqlite3_bind_int(stmt, 18, content.IsNew);
                sqlite3_bind_int(stmt, 19, static_cast<int>(content.LimitedType));
                sqlite3_bind_int(stmt, 20, content.Remaining);
                sqlite3_bind_int(stmt, 21, content.Historical_Sales);
                sqlite3_bind_int(stmt, 22, content.Historical_Favorites);
                sqlite3_bind_int(stmt, 23, content.Historical_Likes);
                sqlite3_bind_int(stmt, 24, content.Historical_Dislikes);
            } else if constexpr (std::is_same_v<T, Badge>) {
                std::string stmtStr = std::format("{} INTO Badge (Id, Name, UserId, GroupId) VALUES(?, ?, ?, ?);", keyword);
                sqlite3_prepare_v2(mDatabase, stmtStr.c_str(), -1,
                                   &stmt, nullptr);
                sqlite3_bind_int64(stmt, 1, content.Id);
                sqlite3_bind_text(stmt, 2, content.Name.c_str(), -1, nullptr);
                content.CreatorType == Roblox::CreatorType::User
                    ? sqlite3_bind_int64(stmt, 3, content.CreatorId)
                    : sqlite3_bind_null(stmt, 3);
                content.CreatorType == Roblox::CreatorType::Group
                    ? sqlite3_bind_int64(stmt, 4, content.CreatorId)
                    : sqlite3_bind_null(stmt, 4);
            }

            int stepRes = sqlite3_step(stmt);
            switch (stepRes) {
                case SQLITE_CONSTRAINT:
                    res = DatabaseResponse::StatementConstraintViolation;
                    break;
                case SQLITE_DONE:
                    MarkDirty();
                    res = DatabaseResponse::Success;
                    break;
                default: break;
            }

            sqlite3_finalize(stmt);
            return res;
        }

        template<typename T>
        std::vector<T> SearchContent(const SearchOptions &opt) {
            static_assert(std::is_base_of_v<IdRecord, T>, "typename must inherit from IdRecord");

            if (!mInitialized) return {};

            std::vector<T> list;

            std::string stmtStr = std::format("SELECT * FROM {} LIMIT ? OFFSET ?;", Reflection::GetIdTypeName<T>());

            if (std::is_same_v<T, Asset> && opt.AssetType != Roblox::AssetType::None)
                stmtStr = std::format("SELECT * FROM {} LIMIT ? OFFSET ? WHERE Type = ?;", Reflection::GetIdTypeName<T>());

            sqlite3_stmt *stmt;
            sqlite3_prepare_v2(mDatabase, stmtStr.c_str(), -1, &stmt, nullptr);
            sqlite3_bind_int(stmt, 1, opt.Limit);
            sqlite3_bind_int(stmt, 2, opt.Offset);

            if (std::is_same_v<T, Asset> && opt.AssetType != Roblox::AssetType::None)
                sqlite3_bind_int(stmt, 3, static_cast<int>(opt.AssetType));

            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const int64_t id = GetValueFromColumnName<int64_t>(stmt, "Id");
                std::optional<T> content = GetContent<T>(id);
                if (content.has_value())
                    list.push_back(content.value());
            }

            sqlite3_finalize(stmt);
            return list;
        }

        template<typename T>
        bool DoesContentExist(const int64_t &id, const int64_t &version = 1) {
            static_assert(std::is_base_of_v<IdRecord, T>, "typename must inherit from IdRecord");
            if (!mInitialized) return false;

            std::string stmtStr = std::format("SELECT Id FROM {} WHERE Id = ? AND Version = ?;", Reflection::GetIdTypeName<T>());

            sqlite3_stmt *stmt;
            sqlite3_prepare_v2(mDatabase, stmtStr.c_str(), -1, &stmt, nullptr);
            sqlite3_bind_int64(stmt, 1, id);
            sqlite3_bind_int(stmt, 2, version);

            int res = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            return res == SQLITE_ROW;
        }
    protected:
        int SetDatabaseVersion(int version);

        std::vector<unsigned char> RetrieveBlobFromTableName(int64_t id, const std::string &tableName, const std::string &columnName);

        // TODO: Optimize this so that it caches the results instead of having to go through the for loop everytime.
        template<typename T>
        T GetValueFromColumnName(sqlite3_stmt *stmt, const std::string &columnName) {
            for (int i = 0; i < sqlite3_column_count(stmt); i++) {
                if (strncmp(sqlite3_column_name(stmt, i), columnName.c_str(), strlen(columnName.c_str())) == 0) {
                    if constexpr (std::is_same_v<T, int> || std::is_same_v<T, bool>)
                        return sqlite3_column_int(stmt, i);
                    if constexpr (std::is_same_v<T, int64_t>)
                        return sqlite3_column_int64(stmt, i);
                    if constexpr (std::is_same_v<T, std::string>) {
                        return std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, i)));
                    }
                    if constexpr (std::is_same_v<T, std::vector<unsigned char>>) {
                        std::vector<unsigned char> data;
                        auto *buf = static_cast<unsigned char*>(const_cast<void*>(sqlite3_column_blob(stmt, i)));
                        data.assign(buf, buf + sqlite3_column_bytes(stmt, i));
                        return data;
                    }
                }
            }
            T def {};
            return def;
        }

        inline int GetTypeFromColumnName(sqlite3_stmt *stmt, const std::string &columnName) {
            for (int i = 0; i < sqlite3_column_count(stmt); i++) {
                if (strncmp(sqlite3_column_name(stmt, i), columnName.c_str(), strlen(columnName.c_str())) == 0) {
                    return sqlite3_column_type(stmt, i);
                }
            }
            return 0;
        }

        inline int GetBlobSizeFromColumnName(sqlite3_stmt *stmt, const std::string &columnName) {
            for (int i = 0; i < sqlite3_column_count(stmt); i++) {
                if (strncmp(sqlite3_column_name(stmt, i), columnName.c_str(), strlen(columnName.c_str())) == 0) {
                    return sqlite3_column_bytes(stmt, i);
                }
            }
            return 0;
        }
    private:
        std::filesystem::path mPath;
        sqlite3 *mDatabase;
        bool mInitialized;
        bool mAutoCommit;
        bool mDirty;
    };
}
