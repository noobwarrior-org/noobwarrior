// === noobWarrior ===
// File: Database.cpp
// Started by: Hattozo
// Started on: 2/17/2025
// Description: Encapsulates a SQLite database and creates tables containing Roblox assets and other kinds of data
#include <NoobWarrior/Database/Database.h>
#include <NoobWarrior/Database/Statement.h>
#include <NoobWarrior/NoobWarrior.h>

#include <nlohmann/json.hpp>

#include <cstdio>
#include <cstring>
#include <sqlite3.h>

#include "../base64.h"

// header files containing strings of SQL schemas describing how each table should be created.
#include "schema/table/meta.sql.inc"
#include "schema/table/blob_storage.sql.inc"
#include "schema/table/login_session.sql.inc"
#include "schema/table/transaction.sql.inc"

#include "schema/table/item/asset.sql.inc"
#include "schema/table/item/badge.sql.inc"
#include "schema/table/item/bundle.sql.inc"
#include "schema/table/item/devproduct.sql.inc"
#include "schema/table/item/group.sql.inc"
#include "schema/table/item/pass.sql.inc"
#include "schema/table/item/set.sql.inc"
#include "schema/table/item/universe.sql.inc"
#include "schema/table/item/user.sql.inc"

#include "schema/table/item/asset/asset_data.sql.inc"
#include "schema/table/item/asset/asset_historical.sql.inc"
#include "schema/table/item/asset/asset_microtransaction.sql.inc"
#include "schema/table/item/asset/asset_misc.sql.inc"
#include "schema/table/item/asset/asset_place_thumbnail.sql.inc"
#include "schema/table/item/asset/asset_place_attributes.sql.inc"
#include "schema/table/item/asset/asset_place_gear_type.sql.inc"

#include "schema/table/item/bundle/bundle_asset.sql.inc"

#include "schema/table/item/universe/universe_misc.sql.inc"
#include "schema/table/item/universe/universe_historical.sql.inc"
#include "schema/table/item/universe/universe_social_link.sql.inc"

#include "schema/table/item/user/user_friends.sql.inc"
#include "schema/table/item/user/user_groups.sql.inc"
#include "schema/table/item/user/user_followers.sql.inc"
#include "schema/table/item/user/user_following.sql.inc"
#include "schema/table/item/user/user_inventory.sql.inc"
#include "schema/table/item/user/user_favorites.sql.inc"
#include "schema/table/item/user/user_likes_dislikes.sql.inc"
#include "schema/table/item/user/user_names.sql.inc"

#include "schema/table/item/group/group_role.sql.inc"
#include "schema/table/item/group/group_wall.sql.inc"
#include "schema/table/item/group/group_log.sql.inc"
#include "schema/table/item/group/group_ally.sql.inc"
#include "schema/table/item/group/group_enemy.sql.inc"
#include "schema/table/item/group/group_historical.sql.inc"
#include "schema/table/item/group/group_social_link.sql.inc"

#include "schema/table/item/set/set_asset.sql.inc"

// sql code for migrating so that when the database file has to be updated it can apply these patches in order.
#include "migrations/v2.sql.inc"

static constexpr const char* MetaKv[][2] = {
	//////////////// Metadata ////////////////
    {"Title", "Untitled"},
    {"Description", "No description available."},
    {"Version", "1.0.0"}, // Not to be confused with the version of the database format, this is meant for the author to set.
	{"Author", "N/A"},
    // The table can only take strings, so we're encoding this in base64.
    // The base64 in this decodes to a default .png file.
    {"Icon", "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"},
	//////////////// Booleans ////////////////
	/** Should parts of the database be able to be modified by players during runtime?
	 * (Ex: Adding records to friend table by friending someone in-game, liking/disliking a game, uploading UGC, etc.)
	 */
	{"Mutable", "1"},
	// If set to a valid compression type, it will compress all binary blobs in the database
	// using the specified compression algorithm. This corresponds to CompressionType enum
	{"CompressionType", "0"},
	// Assets from this database will only be requested if one of your running game servers has loaded a place from this database.
	// No clue how to make this not as wordy as it currently is.
	{"OnlyEnableIfServerWithPlaceFromThisDatabaseIsRunning", "0"},
	// This database will have a higher priority if one of your running game servers has loaded a place from this database.
	// You can turn this on if you are paranoid of conflicting ID's
	// This is even wordier.
	{"TakeHigherPriorityIfServerWithPlaceFromThisDatabaseIsRunning", "0"}
};

using namespace NoobWarrior;

Database::Database(bool autocommit) :
    mDatabase(nullptr),
    mInitialized(false),
	mAutoCommit(autocommit),
	mDirty(false),
	mAssetRepository(this)
{};

DatabaseResponse Database::Open(const std::string &path) {
    mPath = path;
    int val = sqlite3_open_v2(path.c_str(), &mDatabase, SQLITE_OPEN_READWRITE, nullptr);
    if (val != SQLITE_OK)
        return DatabaseResponse::CouldNotOpen;
	if (!mAutoCommit) {
		// disable auto-commit mode by explicitly initiating a transaction
		val = sqlite3_exec(mDatabase, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
		if (val != SQLITE_OK) {
			Out("Database", "Failed to begin new transaction!");
			sqlite3_close_v2(mDatabase);
			return DatabaseResponse::Failed;
		}
	}

	int dbVer = GetDatabaseVersion();
	if (dbVer > 0 && dbVer != NOOBWARRIOR_DATABASE_VERSION) {
		Out("Database", "Database file is out of date because it is on version {}! Must be upgraded to version {}.", dbVer, NOOBWARRIOR_DATABASE_VERSION);
	} else if (dbVer == -1) { // -1 indicates that something has went terribly wrong. how did that happen?
		Out("Database", "Failed to open database file because the version number could not retrieved!");
		sqlite3_close_v2(mDatabase);
        return DatabaseResponse::CouldNotGetVersion;
	}

    switch (dbVer) {
    case 0: // 0 indicates to us that we have a brand new SQLite database, as there is no file version that starts at 0.
        // no migrating required because we are on a clean slate, just set the database version and go
        if (SetDatabaseVersion(NOOBWARRIOR_DATABASE_VERSION) != SQLITE_OK) { sqlite3_close_v2(mDatabase); return DatabaseResponse::CouldNotSetVersion; }
        break;
	case 1: ; // No migration scripts for v1, it's the first version.
    }

#define CREATE_TABLE(var) int var##_execVal = sqlite3_exec(mDatabase, var, nullptr, nullptr, nullptr); \
	if (var##_execVal != SQLITE_OK) { \
		sqlite3_close_v2(mDatabase); \
		Out("Database", "Failed to create table from variable \"{}\"", #var);\
		return DatabaseResponse::CouldNotCreateTable; \
	}

    // create all tables that do not exist
    CREATE_TABLE(schema_meta)
	CREATE_TABLE(schema_blob_storage)
	CREATE_TABLE(schema_login_session)
	CREATE_TABLE(schema_transaction)

	// id type tables
	CREATE_TABLE(schema_asset)
	CREATE_TABLE(schema_badge)
	CREATE_TABLE(schema_bundle)
	CREATE_TABLE(schema_devproduct)
	CREATE_TABLE(schema_group)
	CREATE_TABLE(schema_pass)
	CREATE_TABLE(schema_set)
	CREATE_TABLE(schema_universe)
	CREATE_TABLE(schema_user)

	// tables that are directly related to an id type
	CREATE_TABLE(schema_asset_data)
	CREATE_TABLE(schema_asset_historical)
	CREATE_TABLE(schema_asset_microtransaction)
	CREATE_TABLE(schema_asset_misc)
	CREATE_TABLE(schema_asset_place_thumbnail)
	CREATE_TABLE(schema_asset_place_attributes)
	CREATE_TABLE(schema_asset_place_gear_type)

	CREATE_TABLE(schema_bundle_asset)

	CREATE_TABLE(schema_universe_misc)
	CREATE_TABLE(schema_universe_historical)
	CREATE_TABLE(schema_universe_social_link)

	CREATE_TABLE(schema_user_friends)
	CREATE_TABLE(schema_user_groups)
	CREATE_TABLE(schema_user_followers)
	CREATE_TABLE(schema_user_following)
	CREATE_TABLE(schema_user_inventory)
	CREATE_TABLE(schema_user_favorites)
	CREATE_TABLE(schema_user_likes_dislikes)
	CREATE_TABLE(schema_user_names)

	CREATE_TABLE(schema_group_role)
	CREATE_TABLE(schema_group_wall)
	CREATE_TABLE(schema_group_log)
	CREATE_TABLE(schema_group_ally)
	CREATE_TABLE(schema_group_enemy)
	CREATE_TABLE(schema_group_historical)
	CREATE_TABLE(schema_group_social_link)

	CREATE_TABLE(schema_set_asset)

#undef CREATE_TABLE

    // and initialize some keys
    for (int i = 0; i < NOOBWARRIOR_ARRAY_SIZE(MetaKv); i++) {
    	sqlite3_stmt *checkStmt;
    	if (sqlite3_prepare_v2(mDatabase, "SELECT 1 FROM Meta WHERE Key = ?;", -1, &checkStmt, nullptr) != SQLITE_OK) {
    		Out("checkStmt", "Failed to prepare");
    		sqlite3_finalize(checkStmt);
    		sqlite3_close_v2(mDatabase);
    		return DatabaseResponse::CouldNotSetKeyValues;
    	}
    	sqlite3_bind_text(checkStmt, 1, MetaKv[i][0], -1, nullptr);
    	if (sqlite3_step(checkStmt) == SQLITE_ROW) {
    		// key already exists, don't insert anything, we're done here
    		sqlite3_finalize(checkStmt);
    		continue;
    	}
    	sqlite3_finalize(checkStmt);

    	sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(mDatabase, "INSERT INTO Meta('Key', 'Value') VALUES(?, ?)", -1, &stmt, nullptr) != SQLITE_OK) {
        	Out("Database", "Failed to prepare");
        	sqlite3_finalize(stmt);
        	sqlite3_close_v2(mDatabase);
	        return DatabaseResponse::CouldNotSetKeyValues;
        }
    	sqlite3_bind_text(stmt, 1, MetaKv[i][0], -1, nullptr);
    	sqlite3_bind_text(stmt, 2, MetaKv[i][1], -1, nullptr);
    	if (sqlite3_step(stmt) != SQLITE_DONE) {
    		sqlite3_close_v2(mDatabase);
    		sqlite3_finalize(stmt);
    		return DatabaseResponse::CouldNotSetKeyValues;
    	}
    	sqlite3_finalize(stmt);
    }
    mInitialized = true;
    return DatabaseResponse::Success;
}

int Database::Close() {
    return mDatabase != nullptr ? sqlite3_close_v2(mDatabase) : 0;
}

int Database::GetDatabaseVersion() {
    int val = -1;
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(mDatabase, "PRAGMA user_version;", -1, &stmt, nullptr) != SQLITE_OK)
        goto cleanup;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        val = sqlite3_column_int(stmt, 0); // The first column is user_version.
cleanup:
    sqlite3_finalize(stmt);
    return val;
}

int Database::SetDatabaseVersion(int version) {
	std::string stmtStr = std::format("PRAGMA user_version={}", version);
	int res = sqlite3_exec(mDatabase, stmtStr.c_str(), nullptr, nullptr, nullptr);
	if (res == SQLITE_OK)
		MarkDirty();
    return res;
}

DatabaseResponse Database::SaveAs(const std::string &path) {
	if (!mInitialized) return DatabaseResponse::NotInitialized;
	auto res = DatabaseResponse::Failed;
	sqlite3 *newDb;
	sqlite3_backup *backup;

	FILE *file = fopen(path.c_str(), "w");
	if (file == nullptr)
		return DatabaseResponse::CouldNotOpen;
	fclose(file);

	int val = sqlite3_open_v2(path.c_str(), &newDb, SQLITE_OPEN_READWRITE, nullptr);
	if (val != SQLITE_OK)
		goto cleanup;

	backup = sqlite3_backup_init(newDb, "main", mDatabase, "main");
	if (backup) {
		int backup_step_res = sqlite3_backup_step(backup, -1);
		if (backup_step_res == SQLITE_BUSY) res = DatabaseResponse::Busy;
		if (backup_step_res != SQLITE_DONE && backup_step_res != SQLITE_READONLY) goto cleanup;
		if (sqlite3_backup_finish(backup) != SQLITE_OK) goto cleanup;
		res = DatabaseResponse::Success;
	}
	cleanup:
		sqlite3_close_v2(newDb);
	return res;
}

DatabaseResponse Database::WriteChangesToDisk() {
	if (mAutoCommit) return DatabaseResponse::DidNothing; // You don't have to save
	auto res = DatabaseResponse::Failed;

	sqlite3_stmt *stmt;

	sqlite3_prepare_v2(mDatabase, "COMMIT;", -1, &stmt, nullptr);
	int step_res = sqlite3_step(stmt);
	if (step_res == SQLITE_BUSY) res = DatabaseResponse::Busy;
	if (step_res == SQLITE_MISUSE) res = DatabaseResponse::Misuse;
	if (step_res != SQLITE_DONE) goto cleanup;
	sqlite3_finalize(stmt);

	sqlite3_prepare_v2(mDatabase, "BEGIN TRANSACTION;", -1, &stmt, nullptr);
	step_res = sqlite3_step(stmt);
	if (step_res == SQLITE_BUSY) res = DatabaseResponse::Busy;
	if (step_res == SQLITE_MISUSE) res = DatabaseResponse::Misuse;
	if (step_res != SQLITE_DONE) goto cleanup;

	res = DatabaseResponse::Success;
	UnmarkDirty();
cleanup:
	sqlite3_finalize(stmt);
    return res;
}

bool Database::IsDirty() {
	if (mAutoCommit) return false;
    return mDirty;
}

void Database::MarkDirty() {
	if (mAutoCommit) return; // every single thing is saved, it's never dirty
	mDirty = true;
}

void Database::UnmarkDirty() {
	if (mAutoCommit) return;
	mDirty = false;
}

bool Database::IsMemory() {
	return mPath.compare(":memory:") == 0;
}

bool Database::DoesItemExist(const Reflection::ItemType &itemType, int64_t id, std::optional<int> snapshot) {
	if (!mInitialized) return false;

	std::string stmtStr = std::format("SELECT Id FROM {} WHERE Id = ? {};", itemType.Name, snapshot.has_value() ? "AND Snapshot = ?" : "ORDER BY Snapshot DESC LIMIT 1");

	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(mDatabase, stmtStr.c_str(), -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, id);
	sqlite3_bind_int(stmt, 2, snapshot.value());

	int res = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	return res == SQLITE_ROW;
}

std::string Database::GetSqliteErrorMsg() {
    return sqlite3_errmsg(mDatabase);
}

std::string Database::GetMetaKeyValue(const std::string &key) {
	if (!mInitialized) return "";

	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(mDatabase, "SELECT Value FROM Meta WHERE Key = ?;", -1, &stmt, nullptr);
	sqlite3_bind_text(stmt, 1, key.c_str(), -1, nullptr);

	if (sqlite3_step(stmt) == SQLITE_ROW) {
		auto str = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
		sqlite3_finalize(stmt);
		return str;
	}

	sqlite3_finalize(stmt);
	return "";
}

std::string Database::GetTitle() { return GetMetaKeyValue("Title"); }
std::string Database::GetDescription() { return GetMetaKeyValue("Description"); }
std::string Database::GetVersion() { return GetMetaKeyValue("Version"); }
std::string Database::GetAuthor() { return GetMetaKeyValue("Author"); }
std::vector<unsigned char> Database::GetIcon() { return base64_decode(GetMetaKeyValue("Icon")); }

std::string Database::GetFileName() {
	if (mPath.compare(":memory:"))
		return "Unsaved Database";
	return mPath.filename().string();
}

std::filesystem::path Database::GetFilePath() {
    if (mPath.compare(":memory:"))
        return "";
    return mPath;
}

Statement Database::PrepareStatement(const std::string &stmtStr) {
	return Statement(this, stmtStr);
}

DatabaseResponse Database::ExecStatement(const std::string &stmtStr) {
	int res = sqlite3_exec(mDatabase, stmtStr.c_str(), nullptr, nullptr, nullptr);
	auto ret = DatabaseResponse::Failed;
	switch (res) {
	case SQLITE_OK:
		ret = DatabaseResponse::Success;
		break;
	default: break;
	}
	return ret;
}

DatabaseResponse Database::SetMetaKeyValue(const std::string &key, const std::string &value) {
	if (!mInitialized) return DatabaseResponse::NotInitialized;
	Statement stmt(this, "UPDATE Meta SET Value = ? WHERE Key = ?;");
	stmt.Bind(1, value);
	stmt.Bind(2, key);
	auto res = stmt.Step() == SQLITE_DONE ? DatabaseResponse::Success : DatabaseResponse::Failed;
	MarkDirty();
	return res;
}

DatabaseResponse Database::SetTitle(const std::string &title) {
	return SetMetaKeyValue("Title", title);
}

DatabaseResponse Database::SetAuthor(const std::string &author) {
	return SetMetaKeyValue("Author", author);
}

DatabaseResponse Database::SetIcon(const std::vector<unsigned char> &icon) {
	return SetMetaKeyValue("Icon", base64_encode(icon.data(), icon.size()));
}

AssetRepository& Database::GetAssetRepository() {
	return mAssetRepository;
}

std::vector<unsigned char> Database::RetrieveBlobFromTableName(int64_t id, const std::string &tableName,
	const std::string &columnName) {
	if (!mInitialized) return {};

	std::string stmtStr = std::format("SELECT * FROM {} WHERE Id = ? ORDER BY Snapshot DESC LIMIT 1;", tableName);

	Statement stmt(this, stmtStr);
	stmt.Bind(1, id);

	if (stmt.Step() == SQLITE_ROW) {
		const auto data = GetValueFromColumnName<std::vector<unsigned char>>(stmt.Get(), columnName);
		return data;
	}

	return {};
}
