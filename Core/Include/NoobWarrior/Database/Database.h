// === noobWarrior ===
// File: Database.h
// Started by: Hattozo
// Started on: 2/17/2025
// Description: Encapsulates a SQLite database and creates tables containing Roblox assets and other kinds of data
#pragma once
#include "IdType/Asset.h"
#include "IdType/Badge.h"

#include "../Roblox/Api/Asset.h"
#include "../Log.h"

#include <sqlite3.h>

#include <filesystem>
#include <string>
#include <vector>
#include <cstdint>

#define NOOBWARRIOR_DATABASE_VERSION 1

extern const int g_icon_content_deleted[];
extern const int g_icon_content_deleted_size;

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
    NotInitialized,
    StatementConstraintViolation
};

class Database {
public:
    Database(bool autocommit = true);

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
    std::string GetTitle();
    std::string GetDescription();
    std::string GetVersion();
    std::string GetAuthor();
    std::vector<unsigned char> GetIcon();
    /**
     * @return Returns the file name of the database's currently loaded file.
       If a file is currently not loaded or if the database is stored in memory only it returns a blank string.
       This does not return a file path, do not confuse this function with returning one.
     */
    std::string GetFileName();

    std::filesystem::path GetFilePath();

    DatabaseResponse SetMetaKeyValue(const std::string &key, const std::string &value);
    DatabaseResponse SetTitle(const std::string &title);
    DatabaseResponse SetAuthor(const std::string &author);
    DatabaseResponse SetIcon(const std::vector<unsigned char> &icon);

    template<typename T>
    std::vector<unsigned char> RetrieveContentData(int64_t id) {
    	static_assert(std::is_base_of_v<IdRecord, T>, "typename must inherit from IdRecord");
    	if (!mInitialized) return {};

    	std::string stmtStr = std::format("SELECT * FROM {} WHERE Id = ? ORDER BY Version DESC LIMIT 1;", T::TableName);

    	sqlite3_stmt *stmt;
    	sqlite3_prepare_v2(mDatabase, stmtStr.c_str(), -1, &stmt, nullptr);
    	sqlite3_bind_int64(stmt, 1, id);

    	if (sqlite3_step(stmt) == SQLITE_ROW) {
    		for (int i = 0; i < sqlite3_column_count(stmt); i++) {
    			if (strncmp(sqlite3_column_name(stmt, i), "Data", 4) == 0) {
    				std::vector<unsigned char> data;
    				unsigned char *buf = (unsigned char*)sqlite3_column_blob(stmt, i);
    				data.assign(buf, buf + sqlite3_column_bytes(stmt, i));
    				sqlite3_finalize(stmt);
    				return data;
    			}
    		}
    	}

    	sqlite3_finalize(stmt);

    	return {};
    }

    template<typename T>
	std::vector<unsigned char> RetrieveContentImageData(int64_t id) {
    	static_assert(std::is_base_of_v<IdRecord, T>, "typename must inherit from IdRecord");
    	if (!mInitialized) return {};

    	std::string stmtStr = std::format("SELECT * FROM {} WHERE Id = ? ORDER BY Version DESC LIMIT 1;", T::TableName);

    	sqlite3_stmt *stmt;
    	sqlite3_prepare_v2(mDatabase, stmtStr.c_str(), -1, &stmt, nullptr);
    	sqlite3_bind_int64(stmt, 1, id);
    	if (sqlite3_step(stmt) == SQLITE_ROW) {
    		for (int i = 0; i < sqlite3_column_count(stmt); i++) {
    			if (strncmp(sqlite3_column_name(stmt, i), "Image", 5) == 0) {
    				const int64_t iconId = sqlite3_column_int64(stmt, i);
				    if (std::vector<unsigned char> imageData = RetrieveContentData<Asset>(iconId); !imageData.empty()) {
					    sqlite3_finalize(stmt);
					    return imageData;
				    }
    			}
    		}
    	}
    	sqlite3_finalize(stmt);

    	if constexpr (std::is_same_v<T, Asset>) {
    	}

    	std::vector<unsigned char> data;
    	data.assign(g_icon_content_deleted, g_icon_content_deleted + g_icon_content_deleted_size);
    	return data;
    }

    template<typename T>
    std::optional<T> GetContent(int64_t id) {
        static_assert(std::is_base_of_v<IdRecord, T>, "typename must inherit from IdRecord");
    	if (!mInitialized) return std::nullopt;

        std::string stmtStr = std::format("SELECT * FROM {} WHERE Id = ?;", T::TableName);

        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(mDatabase, stmtStr.c_str(), -1, &stmt, nullptr);
        sqlite3_bind_int64(stmt, 1, id);

        while (sqlite3_step(stmt) != SQLITE_DONE) {
            const char *name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));

            T content {};
            content.Id = sqlite3_column_int(stmt, 0);
            content.Version = sqlite3_column_int(stmt, 1);
            content.FirstRecorded = sqlite3_column_int(stmt, 2);
            content.LastRecorded = sqlite3_column_int(stmt, 3);
            content.Name = std::string(name != nullptr ? name : "No Name");

            if constexpr (std::is_base_of_v<OwnedIdRecord, T>) {
            	const char *desc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));

            	content.Description = std::string(desc != nullptr ? desc : "No description available.");
            	content.Created = sqlite3_column_int(stmt, 6);
            	content.Updated = sqlite3_column_int(stmt, 7);
            }

            if constexpr (std::is_same_v<T, Asset>) {
                const char *thumbnailsJson = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));

				content.Type = static_cast<Roblox::AssetType>(sqlite3_column_int(stmt, 8));
				content.Icon = sqlite3_column_int(stmt, 9);
				try { content.Thumbnails = nlohmann::json::parse(thumbnailsJson != nullptr ? thumbnailsJson : "[]"); } catch (nlohmann::detail::parse_error &e) { content.Thumbnails = "[]"; }
				{
					// The database table has separate fields for the creator ID, "UserId" and "GroupId", so that referencing foreign keys can be possible.
					content.CreatorType = sqlite3_column_type(stmt, 11) != SQLITE_NULL
											? Roblox::CreatorType::User
											: Roblox::CreatorType::Group;
					content.CreatorId = sqlite3_column_type(stmt, 11) != SQLITE_NULL
										  ? sqlite3_column_int(stmt, 11)
										  : sqlite3_column_int(stmt, 12);
				}
				content.PriceInRobux = sqlite3_column_int(stmt, 13);
				content.PriceInTickets = sqlite3_column_int(stmt, 14);
				content.ContentRatingTypeId = sqlite3_column_int(stmt, 15);
				content.MinimumMembershipLevel = sqlite3_column_int(stmt, 16);
				content.IsPublicDomain = sqlite3_column_int(stmt, 17);
				content.IsForSale = sqlite3_column_int(stmt, 18);
				content.IsNew = sqlite3_column_int(stmt, 19);
				content.LimitedType = static_cast<Roblox::LimitedType>(sqlite3_column_int(stmt, 20));
				content.Remaining = sqlite3_column_int(stmt, 21);

				// Historical data
				content.Sales = sqlite3_column_int(stmt, 22);
				content.Favorites = sqlite3_column_int(stmt, 23);
				content.Likes = sqlite3_column_int(stmt, 24);
				content.Dislikes = sqlite3_column_int(stmt, 25);

                return content;
		    } else if constexpr (std::is_same_v<T, Badge>) {
			    return content;
		    }
        }

        sqlite3_finalize(stmt);
        return std::nullopt;
    }

    template<typename T>
    DatabaseResponse AddContent(const T &content) {
        static_assert(std::is_base_of_v<IdRecord, T>, "typename must inherit from IdRecord");

        if (!mInitialized) return DatabaseResponse::NotInitialized;
        DatabaseResponse res = DatabaseResponse::Failed;
        sqlite3_stmt *stmt;

        if constexpr (std::is_same_v<T, Asset>) {
        	sqlite3_prepare_v2(mDatabase, "INSERT INTO Asset (Id, Version, Name, Description, Created, Updated, Type, Image, Thumbnails, UserId, GroupId, PriceInRobux, PriceInTickets, ContentRatingTypeId, MinimumMembershipLevel, IsPublicDomain, IsForSale, IsNew, LimitedType, Remaining, Sales, Favorites, Likes, Dislikes, Data) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);", -1, &stmt, nullptr);
        	sqlite3_bind_int64(stmt, 1, content.Id);
        	sqlite3_bind_int64(stmt, 2, content.Version);
        	sqlite3_bind_text(stmt, 3, content.Name.c_str(), -1, nullptr);
        	sqlite3_bind_text(stmt, 4, content.Description.c_str(), -1, nullptr);
        	sqlite3_bind_int64(stmt, 5, content.Created);
        	sqlite3_bind_int64(stmt, 6, content.Updated);
        	sqlite3_bind_int(stmt, 7, static_cast<int>(content.Type));
        	sqlite3_bind_int64(stmt, 8, content.Icon);
        	sqlite3_bind_text(stmt, 9, content.Thumbnails.dump().c_str(), -1, nullptr);
        	content.CreatorType == Roblox::CreatorType::User ? sqlite3_bind_int64(stmt, 10, content.CreatorId) : sqlite3_bind_null(stmt, 10);
        	content.CreatorType == Roblox::CreatorType::Group ? sqlite3_bind_int64(stmt, 11, content.CreatorId) : sqlite3_bind_null(stmt, 11);
        	sqlite3_bind_int(stmt, 12, content.PriceInRobux);
        	sqlite3_bind_int(stmt, 13, content.PriceInTickets);
        	sqlite3_bind_int(stmt, 14, content.ContentRatingTypeId);
        	sqlite3_bind_int(stmt, 15, content.MinimumMembershipLevel);
        	sqlite3_bind_int(stmt, 16, content.IsPublicDomain);
        	sqlite3_bind_int(stmt, 17, content.IsForSale);
        	sqlite3_bind_int(stmt, 18, content.IsNew);
        	sqlite3_bind_int(stmt, 19, static_cast<int>(content.LimitedType));
        	sqlite3_bind_int(stmt, 20, content.Remaining);
        	sqlite3_bind_int(stmt, 21, content.Sales);
        	sqlite3_bind_int(stmt, 22, content.Favorites);
        	sqlite3_bind_int(stmt, 23, content.Likes);
        	sqlite3_bind_int(stmt, 24, content.Dislikes);
        	sqlite3_bind_blob(stmt, 25, content.Data.data(), content.Data.size(), nullptr);
        } else if constexpr (std::is_same_v<T, Badge>) {
        	sqlite3_prepare_v2(mDatabase, "INSERT INTO Badge (Id, Name, UserId, GroupId) VALUES(?, ?, ?, ?);", -1, &stmt, nullptr);
        	sqlite3_bind_int64(stmt, 1, content.Id);
        	sqlite3_bind_text(stmt, 2, content.Name.c_str(), -1, nullptr);
        	content.CreatorType == Roblox::CreatorType::User ? sqlite3_bind_int64(stmt, 3, content.CreatorId) : sqlite3_bind_null(stmt, 3);
        	content.CreatorType == Roblox::CreatorType::Group ? sqlite3_bind_int64(stmt, 4, content.CreatorId) : sqlite3_bind_null(stmt, 4);
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

    	std::string stmtStr = std::format("SELECT * FROM {} LIMIT ? OFFSET ?;", T::TableName);

	    sqlite3_stmt *stmt;
    	sqlite3_prepare_v2(mDatabase, stmtStr.c_str(), -1, &stmt, nullptr);
    	sqlite3_bind_int(stmt, 1, opt.Limit);
    	sqlite3_bind_int(stmt, 2, opt.Offset);

    	while (sqlite3_step(stmt) != SQLITE_DONE) {
    		const int64_t id = sqlite3_column_int64(stmt, 0);
    		std::optional<T> content = GetContent<T>(id);
    		if (content.has_value())
    			list.push_back(content.value());
    	}

    	sqlite3_finalize(stmt);
    	return list;
    }
protected:
    int SetDatabaseVersion(int version);
private:
    std::filesystem::path mPath;
    sqlite3 *mDatabase;
    bool mInitialized;
    bool mAutoCommit;
    bool mDirty;
};
}