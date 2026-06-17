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
// File: EmuDb.cpp
// Started by: Hattozo
// Started on: 2/17/2025
// Description: Encapsulates a SQLite database and creates tables containing Roblox assets and other kinds of data
// You can find documentation on the file format in the corresponding header file.
#include <NoobWarrior/EmuDb/EmuDb.h>
#include <NoobWarrior/SqlDb/Common.h>
#include <NoobWarrior/SqlDb/Statement.h>
#include <NoobWarrior/NoobWarrior.h>

#include <openssl/sha.h>
#include <sqlite3.h>
#include <zstd.h>
#include <cstdio>

#include "../algorithm/base64.h"
#include "../algorithm/gzip.h"

#include "NoobWarrior/EmuDb/ContentImages.h"
#include "NoobWarrior/EmuDb/ItemType.h"
#include "NoobWarrior/Roblox/Api/Asset.h"

#include "migrations/migration_table.sql.inc.cpp"
#include "migrations/v1.sql.inc.cpp"
#include "migrations/v2.sql.inc.cpp"
#include "migrations/v3.sql.inc.cpp"
#include "migrations/v4.sql.inc.cpp"
#include "migrations/v5.sql.inc.cpp"
#include "migrations/v6.sql.inc.cpp"
#include "migrations/v7.sql.inc.cpp"
#include "migrations/v8.sql.inc.cpp"
#include "migrations/v9.sql.inc.cpp"
#include "migrations/v10.sql.inc.cpp"
#include "migrations/v11.sql.inc.cpp"
#include "migrations/v12.sql.inc.cpp"
#include "migrations/v13.sql.inc.cpp"
#include "migrations/v14.sql.inc.cpp"
#include "migrations/v15.sql.inc.cpp"
#include "migrations/v16.sql.inc.cpp"

using namespace NoobWarrior;

std::vector<unsigned char> EmuDb::RetrieveAssetTypeImageData(Roblox::AssetType type) {
	std::vector<unsigned char> imgData;
	switch (type) {
	default:
		imgData.assign(g_icon_content_deleted, g_icon_content_deleted + g_icon_content_deleted_size);
		break;
	case Roblox::AssetType::Model:
		imgData.assign(g_model_png, g_model_png + g_model_png_size);
		break;
	case Roblox::AssetType::Audio:
		imgData.assign(g_audio_png, g_audio_png + g_audio_png_size);
		break;
	case Roblox::AssetType::Animation:
		imgData.assign(g_animation_png, g_animation_png + g_animation_png_size);
		break;
	case Roblox::AssetType::Video:
		imgData.assign(g_animation_png, g_animation_png + g_animation_png_size);
		break;
	}
	return imgData;
}

bool EmuDb::IsZstdCompressed(const std::vector<unsigned char>& data) {
    if (data.size() < 4) return false;
    return data[0] == 0x28 && data[1] == 0xB5 && data[2] == 0x2F && data[3] == 0xFD;
}

EmuDb::EmuDb(const std::string &path, bool autocommit) :
	SqlDb(path, "EmuDb"),
	mAutoCommit(autocommit),
	mDirty(false),
	mMigrationFailMsg("no failure detected")
{
	if (Fail())
		return;

	bool fts5_enabled = false;
	SqlRows compileOptions = GetPragma("compile_options");
	for (SqlRow compileOption : compileOptions) {
		SqlColumn column = compileOption.at(0);
		const std::string *str = std::get_if<std::string>(&column.second);
		if (str != nullptr) {
			if (str->compare("ENABLE_FTS5") == 0)
				fts5_enabled = true;
		}
	}
	if (!fts5_enabled) {
		Out("Fatal error: FTS5 is not enabled! This is required for search indexing to work. Aborting!\nIf you are the developer, please include it in your build of SQLite.");
		mFailReason = FailReason::FeatureUnavailable;
		return;
	}

	// if auto-commit is disabled, explicitly initiate a transaction
	if (!mAutoCommit && !ExecStatement("BEGIN TRANSACTION")) {
		Out("Failed to begin new transaction. Aborting!");
		mFailReason = FailReason::TransactionFailed;
		return;
	}

	if (!MigrateToLatestVersion()) {
		Out("Failed to migrate to latest version. The database has possibly been corrupted. Now aborting.");
		mFailReason = FailReason::MigrationFailed;
		return;
	}

	if (GetMetaKeyValue("Icon").empty()) {
		std::vector<unsigned char> imgData;
		imgData.assign(g_database_png, g_database_png + g_database_png_size);
		SetIcon(imgData);
	}

	if (mThisIsANewFileOnDisk && !mAutoCommit) {
		// Remember that we just migrated the database and everything in this new file, so the DB is already dirty if auto-commit is disabled.
		// Since we have a new file it is currently at zero bytes.
		// We need to auto-save it for the user because they just created the file and it would be weird to have an unsaved zero byte file.
		WriteChangesToDisk();
	}

	mFailReason = FailReason::None;
};

bool EmuDb::VerifyIntegrityOfMigration() {
	Statement stmt = PrepareStatement("SELECT * FROM Migration");
	if (stmt.Fail()) {
		// and what do you do if you're the user? NOTHING
		mMigrationFailMsg = GetLastErrorMsg();
		Out("Failed to verify integrity of migration: select statement failed. The Migration table most likely does not exist. Fail message: {}", mMigrationFailMsg);
		return false;
	}

	int prevRowId = 0;
	int prevVer = 0;
	sqlite3_int64 prevTimestamp = 0;

	while (1) {
		int step = stmt.Step();
		if (step == SQLITE_ROW) {
			int rowId = sqlite3_column_int(stmt.Get(), 0);
			std::string version = (char*)sqlite3_column_text(stmt.Get(), 1);
			sqlite_int64 timestamp = sqlite3_column_int64(stmt.Get(), 2);

			int verToStr;
			try {
				if (version.starts_with("v")) // Cut out the "v" portion of "v1" because we want to make it a valid number.
					version = version.substr(1, std::string::npos);
				verToStr = std::stoi(version);
			} catch (std::exception &ex) {
				mMigrationFailMsg = "Failed to verify integrity of migration: cannot convert version string to number!";
				Out(mMigrationFailMsg);
				return false;
			}

			if (verToStr != rowId) {
				mMigrationFailMsg = std::format("Failed to verify integrity of migration: version {} does not match row ID {}. Did the developer order the versions wrong? Is there a gap?", version, rowId);
				Out(mMigrationFailMsg);
				return false;
			}

			if (rowId > prevRowId && prevVer > verToStr) {
				mMigrationFailMsg = std::format("Failed to verify integrity of migration: the newer version {} has a lower number than previous version {}. Did the developer order the versions wrong?", version, prevVer);
				Out(mMigrationFailMsg);
				return false;
			}

			prevRowId = rowId;
			prevVer = verToStr;
			prevTimestamp = timestamp;
		} else {
			if (step != SQLITE_DONE) {
				mMigrationFailMsg = GetLastErrorMsg();
				Out("Failed to verify integrity of migration: could not select from migration table. Maybe it doesn't exist?\nFull error: {}", mMigrationFailMsg);
			}
			return step == SQLITE_DONE;
		}
	}
}

bool EmuDb::MigrateToLatestVersion() {
	/* Do NOT begin a transaction if we are not in auto-commit mode because it's already in a transaction.
	   And plus, this happens automatically & we don't want to overwrite the file without user confirmation */
	if (mAutoCommit) {
		bool transactionStmt = ExecStatement("BEGIN TRANSACTION;");
		if (!transactionStmt) {
			mMigrationFailMsg = GetLastErrorMsg();
			Out("Failed to begin new transaction in order to perform migration: \"{}\"", mMigrationFailMsg);
			return false;
		}
	}

#define CREATE_TABLE(var) \
	if (!ExecStatement(var)) { \
		mMigrationFailMsg = std::format("Failed to create table from variable \"{}\"", #var); \
		Out(mMigrationFailMsg); \
		return false; \
	}

	CREATE_TABLE(schema_migration);

	if (!VerifyIntegrityOfMigration()) {
		Out("Failed to begin migration because the integrity check failed.");
		return false;
	}

	bool bindingsSet = false;
	Statement migrationStmt = PrepareStatement("SELECT RowId, Version FROM Migration WHERE Version = ?");
	if (migrationStmt.Fail()) {
		mMigrationFailMsg = GetLastErrorMsg();
		Out("Failed to prepare select statement for Migration table: \"{}\"", mMigrationFailMsg);
		return false;
	}

#define MIGRATE(migration) \
	if (bindingsSet) { \
		if (migrationStmt.Reset() != SQLITE_OK) { Out("Failed to reset selecting migration statement"); return false; } \
		if (migrationStmt.ClearBindings() != SQLITE_OK) { Out("Failed to clear bindings for selecting migration statement"); return false; } \
	} \
	if (migrationStmt.Bind(1, #migration) != SQLITE_OK) { Out("Failed to bind value to selecting migration statement"); return false; }; \
	bindingsSet = true; \
	if (migrationStmt.Step() == SQLITE_DONE) { \
		bool success = ExecStatement(migration_##migration); \
		if (success) { \
			Statement addToListStmt = PrepareStatement("INSERT INTO Migration (Version) VALUES (?)"); \
			addToListStmt.Bind(1, #migration); \
			if (addToListStmt.Step() == SQLITE_DONE) { Out("Migrated to " #migration); MarkDirty(); } \
			else { Out("Failed to insert row into migraton table. Message: \"{}\"", GetLastErrorMsg()); return false; } \
		} else { \
			mMigrationFailMsg = std::format("Migration to " #migration " failed: \"{}\"", GetLastErrorMsg()); \
			Out(mMigrationFailMsg); \
			return false; \
		} \
	}

	/** All of this is done in order. DO IT IN THE RIGHT ORDER OR YOU'RE FUCKED!!!!!!! **/
	/* V1: Adds a few important tables like Meta, BlobStorage, and LoginSession */
	MIGRATE(v1)
	/* V2: Adds Transaction and FsNode table */
	MIGRATE(v2)
	/* V3: Adds all of the most important Roblox stuff, like Asset, Badge, Bundle, DevProduct, Group, Pass, etc. */
	MIGRATE(v3)
	/* V4: enables enforcement of foreign keys */
	MIGRATE(v4)
	/* V5: added search index for assets */
	MIGRATE(v5)
	/* V6: added outfits table */
	MIGRATE(v6)
	/* V7: merged AssetMisc table with Asset. Also added new character appearance system for users */
	MIGRATE(v7)
    /* V8: added tables for forums */
    MIGRATE(v8)
    /* V9: added UniversePlace table because that didn't exist before for some reason.
     * Also removed Description column from Universe table */
    MIGRATE(v9)
	/* V10: added Email and PasswordHash fields to User table */
	MIGRATE(v10)
	/* V11: dropped AssetPlaceThumbnail table and remade it with AutogeneratedThumbnailHash field */
	MIGRATE(v11)
	/* V12: added PasswordSalt field to User table */
	MIGRATE(v12)
	/* V13: Added ForumCategory table and made fields in Forum table able to be parented to it */
	MIGRATE(v13)
	/* V14: recreated AssetPlaceThumbnail with a composite key so a place can have multiple thumbnails */
	MIGRATE(v14)
	/* V15: recreated AssetPlaceGearType with a composite key so a place can permit multiple gear types */
	MIGRATE(v15)
	/* V16: recreated UniverseSocialLink keyed by (Id, LinkType) so a universe can have multiple social links */
	MIGRATE(v16)

	// TODO: only do this when we migrate to zstandard
	/* V4: Sets CompressionType value in Meta table to 1, which corresponds to CompressionType::ZStandard.
	   In other words, the default for compressing files has changed to the zstandard format.
	   Note that zstd was not implemented in noobWarrior by the time this change was created.
	MIGRATE(v4) */

#undef MIGRATE
#undef CREATE_TABLE

	if (!VerifyIntegrityOfMigration()) {
		Out("Failed to finish migration because the integrity check failed. Changes will not be saved.");
		return false;
	}

	if (mAutoCommit) {
		bool commitStmt = ExecStatement("COMMIT;");
		if (!commitStmt) {
			Out("Failed to commit transaction in order to complete migration. Changes will not be saved.");
			return false;
		}
	}
	return true;
}

int EmuDb::GetMigrationVersion() {
    int val = -1;
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(mDb, "PRAGMA user_version;", -1, &stmt, nullptr) != SQLITE_OK)
        goto cleanup;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        val = sqlite3_column_int(stmt, 0); // The first column is user_version.
cleanup:
    sqlite3_finalize(stmt);
    return val;
}

std::string EmuDb::GetMigrationFailMsg() {
	return mMigrationFailMsg;
}

SqlDb::Response EmuDb::SaveAs(const std::string &path) {
	if (Fail()) return SqlDb::Response::DatabaseFailed;
	auto res = SqlDb::Response::Failed;
	sqlite3 *newDb;
	sqlite3_backup *backup;

	FILE *file = fopen(path.c_str(), "w");
	if (file == nullptr)
		return SqlDb::Response::CantOpen;
	fclose(file);

	int val = sqlite3_open_v2(path.c_str(), &newDb, SQLITE_OPEN_READWRITE, nullptr);
	if (val != SQLITE_OK)
		goto cleanup;

	backup = sqlite3_backup_init(newDb, "main", mDb, "main");
	if (backup) {
		int backup_step_res = sqlite3_backup_step(backup, -1);
		if (backup_step_res == SQLITE_BUSY) res = SqlDb::Response::Busy;
		if (backup_step_res != SQLITE_DONE && backup_step_res != SQLITE_READONLY) goto cleanup;
		if (sqlite3_backup_finish(backup) != SQLITE_OK) goto cleanup;
		res = SqlDb::Response::Success;
	}
	cleanup:
		sqlite3_close_v2(newDb);
	return res;
}

SqlDb::Response EmuDb::WriteChangesToDisk() {
	if (mAutoCommit) return SqlDb::Response::DidNothing; // You don't have to save
	auto res = SqlDb::Response::Failed;

	sqlite3_stmt *stmt;

	sqlite3_prepare_v2(mDb, "COMMIT;", -1, &stmt, nullptr);
	int step_res = sqlite3_step(stmt);
	if (step_res == SQLITE_BUSY) res = SqlDb::Response::Busy;
	if (step_res == SQLITE_MISUSE) res = SqlDb::Response::Misuse;
	if (step_res != SQLITE_DONE) goto cleanup;
	sqlite3_finalize(stmt);

	sqlite3_prepare_v2(mDb, "BEGIN TRANSACTION;", -1, &stmt, nullptr);
	step_res = sqlite3_step(stmt);
	if (step_res == SQLITE_BUSY) res = SqlDb::Response::Busy;
	if (step_res == SQLITE_MISUSE) res = SqlDb::Response::Misuse;
	if (step_res != SQLITE_DONE) goto cleanup;

	res = SqlDb::Response::Success;
	UnmarkDirty();
cleanup:
	sqlite3_finalize(stmt);
    return res;
}

bool EmuDb::IsDirty() {
	if (mAutoCommit) return false;
    return mDirty;
}

void EmuDb::MarkDirty() {
	if (mAutoCommit) return; // every single thing is saved, it's never dirty
	mDirty = true;
}

void EmuDb::UnmarkDirty() {
	if (mAutoCommit) return;
	mDirty = false;
}

std::string EmuDb::GetMetaKeyValue(const std::string &key) {
	if (Fail()) return "";

	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(mDb, "SELECT Value FROM Meta WHERE Key = ?;", -1, &stmt, nullptr);
	sqlite3_bind_text(stmt, 1, key.c_str(), -1, nullptr);

	if (sqlite3_step(stmt) == SQLITE_ROW) {
		auto str = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
		sqlite3_finalize(stmt);
		return str;
	}

	sqlite3_finalize(stmt);
	return "";
}

std::string EmuDb::GetTitle() { return GetMetaKeyValue("Title"); }
std::string EmuDb::GetDescription() { return GetMetaKeyValue("Description"); }
std::string EmuDb::GetVersion() { return GetMetaKeyValue("Version"); }
std::string EmuDb::GetAuthor() { return GetMetaKeyValue("Author"); }
std::vector<unsigned char> EmuDb::GetIcon() { return base64_decode(GetMetaKeyValue("Icon")); }
EmuDb::CompressionType EmuDb::GetCompressionType() {
    std::string str = GetMetaKeyValue("CompressionType");
    int val = strtol(str.c_str(), nullptr, 10);
    return static_cast<CompressionType>(val);
}

SqlDb::Response EmuDb::SetMetaKeyValue(const std::string &key, const std::string &value) {
	if (Fail()) return SqlDb::Response::DatabaseFailed;
	Statement stmt(this, "UPDATE Meta SET Value = ? WHERE Key = ?;");
	stmt.Bind(1, value);
	stmt.Bind(2, key);
	auto res = stmt.Step() == SQLITE_DONE ? SqlDb::Response::Success : SqlDb::Response::Failed;
	MarkDirty();
	return res;
}

SqlDb::Response EmuDb::SetTitle(const std::string &title) {
	return SetMetaKeyValue("Title", title);
}

SqlDb::Response EmuDb::SetDescription(const std::string &desc) {
	return SetMetaKeyValue("Description", desc);
}

SqlDb::Response EmuDb::SetVersion(const std::string &ver) {
	return SetMetaKeyValue("Version", ver);
}

SqlDb::Response EmuDb::SetAuthor(const std::string &author) {
	return SetMetaKeyValue("Author", author);
}

SqlDb::Response EmuDb::SetIcon(const std::vector<unsigned char> &icon) {
	return SetMetaKeyValue("Icon", base64_encode(icon.data(), icon.size()));
}

#define CHECK_STMT(stmt) \
	if (stmt.Fail()) { \
		Out("Failed to prepare SQL statement. Message: \"{}\"", GetLastErrorMsg()); \
		return SqlDb::Response::Failed; \
	}

/* Note: This just adds it to the storage. */
SqlDb::Response EmuDb::AddBlob(const std::vector<unsigned char> &data, std::string *hashOutput) {
	if (Fail()) return SqlDb::Response::DatabaseFailed;
    if (data.size() >= 2147483648) {
        return SqlDb::Response::BlobTooLarge;
    }

	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256(data.data(), data.size(), hash);

	std::string hashStr;
	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
		hashStr += std::format("{:02x}", hash[i]);
	}

	Statement checkStmt = PrepareStatement("SELECT * FROM BlobStorage WHERE Hash = ?;");
	CHECK_STMT(checkStmt)
	checkStmt.Bind(1, hashStr);
	if (checkStmt.Step() == SQLITE_ROW) {
		if (hashOutput != nullptr)
			*hashOutput = hashStr;
		return SqlDb::Response::DidNothing;
	}

    std::vector<unsigned char> zstd_compressed_data;
    if (GetCompressionType() == CompressionType::ZStandard) {
		size_t bufSize = ZSTD_compressBound(data.size());
		zstd_compressed_data.resize(bufSize);
		size_t newSize = ZSTD_compress(
			zstd_compressed_data.data(), bufSize,
			data.data(), data.size(),
			3
		);
		if (ZSTD_isError(newSize)) {
			Out(std::format("ZSTD compression failed: {}", ZSTD_getErrorName(newSize)));
			return SqlDb::Response::Failed;
		}
		zstd_compressed_data.resize(newSize); // trim to actual compressed size
	}

	Statement stmt = PrepareStatement("INSERT INTO BlobStorage (Hash, Blob) VALUES (?, ?);");
	CHECK_STMT(stmt)
	stmt.Bind(1, hashStr);
    if (GetCompressionType() != CompressionType::ZStandard) {
    	stmt.Bind(2, data);
	} else if (GetCompressionType() == CompressionType::ZStandard) {
	    stmt.Bind(2, zstd_compressed_data);
	}

	switch (stmt.Step()) {
	default: return SqlDb::Response::Failed;
	case SQLITE_DONE:
		if (hashOutput != nullptr)
			*hashOutput = hashStr;
		return SqlDb::Response::Success;
	case SQLITE_BUSY: return SqlDb::Response::Busy;
	case SQLITE_MISUSE: return SqlDb::Response::Misuse;
	case SQLITE_CONSTRAINT: return SqlDb::Response::ConstraintViolation;
	}
}

SqlDb::Response EmuDb::AddBlob(const std::filesystem::path &path, std::string *hashOutput) {
    if (Fail()) return SqlDb::Response::DatabaseFailed;

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return SqlDb::Response::CantOpen;

    uintmax_t fileSize = 0;
    try {
        fileSize = std::filesystem::file_size(path);
        if (fileSize >= 2147483648) return SqlDb::Response::BlobTooLarge;
    } catch (std::filesystem::filesystem_error &) {
        return SqlDb::Response::CantOpen;
    }

    std::vector<unsigned char> rawData(fileSize);
    if (!file.read(reinterpret_cast<char*>(rawData.data()), fileSize))
        return SqlDb::Response::CantOpen;

    return AddBlob(rawData, hashOutput);
}

SqlDb::Response EmuDb::AddItem(ItemType type, SqlRow row) {
	if (Fail()) return SqlDb::Response::DatabaseFailed;

	std::string tableName = GetTableNameFromItemType(type);

	// Drop columns the table doesn't actually have so a schema-drifted database (an older file missing
	// a column a newer build expects) doesn't fail the whole insert. If the schema can't be read, keep
	// the row as-is rather than discarding everything.
	std::set<std::string> existingColumns = GetColumnNames(tableName);
	if (!existingColumns.empty()) {
		SqlRow kept;
		for (const SqlColumn &col : row) {
			if (existingColumns.count(col.first))
				kept.push_back(col);
			else
				Out("Skipping column \"{}\" not present in table {} (schema drift?)", col.first, tableName);
		}
		row = std::move(kept);
	}
	if (row.empty())
		return SqlDb::Response::DidNothing;

	std::string stmtStr = "INSERT INTO " + tableName + " (";

	int values = 0;
	for (int i = 0; i < row.size(); i++) {
		SqlColumn column = row.at(i);
		std::string columnName = column.first;
		stmtStr += columnName + (i != row.size() - 1 ? ", " : "");
		values++;
	}

	stmtStr += ") VALUES (";
	for (int i = 0; i < values; i++)
		stmtStr += std::string("?") + (i != values - 1 ? ", " : ");");

	Out("Adding item using statement \"{}\"", stmtStr);
	Statement stmt = PrepareStatement(stmtStr);
	if (stmt.Fail()) {
		Out("Failed to prepare statement");
		return SqlDb::Response::Failed;
	}

	for (int i = 0; i < row.size(); i++) {
		SqlColumn column = row.at(i);
		SqlValue columnValue = column.second;
		stmt.Bind(i + 1, columnValue);
	}

	switch (stmt.Step()) {
	default:
		return SqlDb::Response::Failed;
	case SQLITE_DONE:
		return SqlDb::Response::Success;
	case SQLITE_BUSY:
		return SqlDb::Response::Busy;
	case SQLITE_MISUSE:
		return SqlDb::Response::Misuse;
	case SQLITE_CONSTRAINT:
		return SqlDb::Response::ConstraintViolation;
	}
}

SqlDb::Response EmuDb::UpdateItem(ItemType type, int64_t id, SqlRow row) {
	if (Fail()) return SqlDb::Response::DatabaseFailed;

	std::string tableName = GetTableNameFromItemType(type);

	// Skip columns the table doesn't have (schema drift), mirroring AddItem.
	std::set<std::string> existingColumns = GetColumnNames(tableName);
	if (!existingColumns.empty()) {
		SqlRow kept;
		for (const SqlColumn &col : row) {
			if (existingColumns.count(col.first))
				kept.push_back(col);
			else
				Out("Skipping column \"{}\" not present in table {} (schema drift?)", col.first, tableName);
		}
		row = std::move(kept);
	}
	if (row.empty())
		return SqlDb::Response::DidNothing;

	std::string stmtStr = "UPDATE " + tableName + " SET ";

	for (int i = 0; i < row.size(); i++) {
		SqlColumn column = row.at(i);
		std::string columnName = column.first;
		stmtStr += columnName + " = ?" + (i != row.size() - 1 ? ", " : "");
	}

	stmtStr += " WHERE Id = ?;";

	Out("Updating item using statement \"{}\"", stmtStr);
	Statement stmt = PrepareStatement(stmtStr);
	if (stmt.Fail()) {
		Out("Failed to prepare statement");
		return SqlDb::Response::Failed;
	}

	int i = 0;
	for (i = 0; i < row.size(); i++) {
		SqlColumn column = row.at(i);
		SqlValue columnValue = column.second;
		stmt.Bind(i + 1, columnValue);
	}
	stmt.Bind(i + 1, id);

	switch (stmt.Step()) {
	default:
		return SqlDb::Response::Failed;
	case SQLITE_DONE:
		return SqlDb::Response::Success;
	case SQLITE_BUSY:
		return SqlDb::Response::Busy;
	case SQLITE_MISUSE:
		return SqlDb::Response::Misuse;
	case SQLITE_CONSTRAINT:
		return SqlDb::Response::ConstraintViolation;
	}
}

std::vector<std::string> EmuDb::GetTableNames() {
	std::vector<std::string> names;
	Statement stmt = PrepareStatement("SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%';");
	if (stmt.Fail()) return names;
	while (stmt.Step() == SQLITE_ROW)
		names.push_back(stmt.GetStringFromColumnIndex(0));
	return names;
}

std::set<std::string> EmuDb::GetColumnNames(const std::string &table) {
	std::set<std::string> cols;
	// PRAGMA table_info columns: 0=cid 1=name 2=type 3=notnull 4=dflt_value 5=pk
	Statement stmt = PrepareStatement(std::format("PRAGMA table_info(\"{}\");", table));
	if (stmt.Fail()) return cols;
	while (stmt.Step() == SQLITE_ROW)
		cols.insert(stmt.GetStringFromColumnIndex(1));
	return cols;
}

std::vector<std::string> EmuDb::GetBlobHashColumns(const std::string &table) {
	std::vector<std::string> cols;
	// PRAGMA foreign_key_list columns: 0=id 1=seq 2=table(referenced) 3=from 4=to 5=on_update 6=on_delete 7=match
	Statement stmt = PrepareStatement(std::format("PRAGMA foreign_key_list(\"{}\");", table));
	if (stmt.Fail()) return cols;
	while (stmt.Step() == SQLITE_ROW) {
		if (stmt.GetStringFromColumnIndex(2) == "BlobStorage")
			cols.push_back(stmt.GetStringFromColumnIndex(3));
	}
	return cols;
}

void EmuDb::CollectRowBlobHashes(const std::string &table, const std::string &whereColumn, int64_t id,
                                 std::set<std::string> &out) {
	std::vector<std::string> blobCols = GetBlobHashColumns(table);
	if (blobCols.empty()) return;

	std::string cols;
	for (size_t i = 0; i < blobCols.size(); i++) {
		if (i) cols += ", ";
		cols += std::format("\"{}\"", blobCols[i]);
	}

	Statement stmt = PrepareStatement(std::format("SELECT {} FROM \"{}\" WHERE \"{}\" = ?;", cols, table, whereColumn));
	if (stmt.Fail()) return;
	stmt.Bind(1, id);
	while (stmt.Step() == SQLITE_ROW) {
		for (int i = 0; i < static_cast<int>(blobCols.size()); i++) {
			if (stmt.IsColumnIndexNull(i)) continue;
			std::string hash = stmt.GetStringFromColumnIndex(i);
			if (!hash.empty()) out.insert(hash);
		}
	}
}

SqlDb::Response EmuDb::DeleteItem(ItemType type, int64_t id) {
	if (Fail()) return SqlDb::Response::DatabaseFailed;

	const std::string parentTable = GetTableNameFromItemType(type);

	// Foreign keys aren't enforced on this connection and none declare ON DELETE CASCADE, so a bare
	// DELETE on the parent table would orphan every dependent row (AssetData versions, the place
	// detail tables, junction rows like OwnedItem/BundleAsset/Transaction) and leak the
	// content-addressed blobs those rows kept alive. Discover dependents from the live schema's
	// foreign keys and remove them ourselves, then garbage-collect any blob that is now unreferenced.

	// Collect the (table, column) pairs that reference the parent before touching any data, so the
	// PRAGMA cursors are fully consumed before we start issuing DELETEs.
	struct Dependent { std::string table; std::string column; };
	std::vector<Dependent> dependents;
	for (const std::string &childTable : GetTableNames()) {
		if (childTable == parentTable) continue;
		Statement fkStmt = PrepareStatement(std::format("PRAGMA foreign_key_list(\"{}\");", childTable));
		if (fkStmt.Fail()) continue;
		while (fkStmt.Step() == SQLITE_ROW) {
			if (fkStmt.GetStringFromColumnIndex(2) == parentTable)
				dependents.push_back({childTable, fkStmt.GetStringFromColumnIndex(3)});
		}
	}

	if (!ExecStatement("SAVEPOINT DeleteItemCascade")) {
		Out("Failed to open savepoint while deleting {} id {}", parentTable, id);
		return SqlDb::Response::Failed;
	}

	// Hashes of blobs the deleted rows referenced; GC'd only after every delete lands so the orphan
	// check in GarbageCollectBlobIfOrphaned sees the final state of the database.
	std::set<std::string> touchedBlobHashes;
	CollectRowBlobHashes(parentTable, "Id", id, touchedBlobHashes);

	bool ok = true;

	// Delete dependents first (one level deep, which covers the current schema: all dependents are
	// leaf detail/junction tables). A self-referencing or many-column dependent simply yields several
	// entries here and each is cleared independently.
	for (const Dependent &dep : dependents) {
		CollectRowBlobHashes(dep.table, dep.column, id, touchedBlobHashes);

		Statement del = PrepareStatement(std::format("DELETE FROM \"{}\" WHERE \"{}\" = ?;", dep.table, dep.column));
		if (del.Fail()) { ok = false; break; }
		del.Bind(1, id);
		if (del.Step() != SQLITE_DONE) {
			Out("Failed to delete dependents from {} for {} id {}: \"{}\"", dep.table, parentTable, id, GetLastErrorMsg());
			ok = false;
			break;
		}
	}

	if (ok) {
		Statement del = PrepareStatement(std::format("DELETE FROM \"{}\" WHERE Id = ?;", parentTable));
		if (del.Fail()) {
			ok = false;
		} else {
			del.Bind(1, id);
			if (del.Step() != SQLITE_DONE) {
				Out("Failed to delete {} id {}: \"{}\"", parentTable, id, GetLastErrorMsg());
				ok = false;
			}
		}
	}

	if (!ok) {
		ExecStatement("ROLLBACK TO DeleteItemCascade");
		ExecStatement("RELEASE DeleteItemCascade");
		return SqlDb::Response::Failed;
	}

	if (!ExecStatement("RELEASE DeleteItemCascade")) {
		Out("Failed to release savepoint after deleting {} id {}", parentTable, id);
		return SqlDb::Response::Failed;
	}

	for (const std::string &hash : touchedBlobHashes)
		GarbageCollectBlobIfOrphaned(hash);

	MarkDirty();
	return SqlDb::Response::Success;
}

bool EmuDb::DoesItemExist(ItemType type, int64_t id) {
    if (Fail()) return false;

    std::string tableName = GetTableNameFromItemType(type);

    Statement stmt = PrepareStatement("SELECT Id FROM " + tableName + " WHERE Id = ?;");
    stmt.Bind(1, id);
    return stmt.Step() == SQLITE_ROW;
}

SqlDb::Response EmuDb::AttachDataToAsset(int64_t id, int version, const std::vector<unsigned char> &data) {
	if (Fail()) return SqlDb::Response::DatabaseFailed;

	std::string hashStr;
	SqlDb::Response res = AddBlob(data, &hashStr);
	if (res != SqlDb::Response::Success && res != SqlDb::Response::DidNothing) {
		Out("Failed to attach data to asset id {} because the blob could not be added to the database", id);
		return SqlDb::Response::Failed;
	}

    return AttachBlobHashToAsset(id, version,  hashStr);
}

void EmuDb::GarbageCollectBlobIfOrphaned(const std::string &hash) {
	if (hash.empty()) return;

	static constexpr const char* kRefs[][2] = {
		{"AssetData", "DataHash"},
		{"AssetData", "AutogeneratedThumbnailHash"},
		{"AssetPlaceThumbnail", "AutogeneratedThumbnailHash"},
		{"User", "HeadshotThumbnailHash"},
		{"User", "BustThumbnailHash"},
	};
	for (const auto &ref : kRefs) {
		Statement stmt = PrepareStatement(std::format("SELECT 1 FROM {} WHERE {} = ? LIMIT 1;", ref[0], ref[1]));
		if (stmt.Fail()) return;
		stmt.Bind(1, hash);
		if (stmt.Step() == SQLITE_ROW)
			return;
	}

	Statement del = PrepareStatement("DELETE FROM BlobStorage WHERE Hash = ?;");
	if (del.Fail()) return;
	del.Bind(1, hash);
	del.Step();
}

SqlDb::Response EmuDb::DetachDataFromAsset(int64_t id, int version) {
	if (Fail()) return SqlDb::Response::DatabaseFailed;

	if (version <= 0) {
		Statement maxStmt = PrepareStatement("SELECT MAX(Version) FROM AssetData WHERE Id = ?;");
		CHECK_STMT(maxStmt)
		maxStmt.Bind(1, id);
		if (maxStmt.Step() != SQLITE_ROW || maxStmt.IsColumnIndexNull(0))
			return SqlDb::Response::NotFound;
		version = maxStmt.GetIntFromColumnIndex(0);
	}

	std::string hash;
	{
		Statement hashStmt = PrepareStatement("SELECT DataHash FROM AssetData WHERE Id = ? AND Version = ?;");
		CHECK_STMT(hashStmt)
		hashStmt.Bind(1, id);
		hashStmt.Bind(2, version);
		if (hashStmt.Step() != SQLITE_ROW)
			return SqlDb::Response::NotFound;
		hash = hashStmt.GetStringFromColumnIndex(0);
	}

	Statement delStmt = PrepareStatement("DELETE FROM AssetData WHERE Id = ? AND Version = ?;");
	CHECK_STMT(delStmt)
	delStmt.Bind(1, id);
	delStmt.Bind(2, version);
	if (delStmt.Step() != SQLITE_DONE) {
		Out("Failed to detach data v{} from asset id {}. Message: \"{}\"", version, id, GetLastErrorMsg());
		return SqlDb::Response::Failed;
	}

	GarbageCollectBlobIfOrphaned(hash);
	MarkDirty();
	return SqlDb::Response::Success;
}

SqlDb::Response EmuDb::AttachBlobHashToAsset(int64_t id, int version, const std::string &hash) {
    if (version > 0) {
		Statement checkStmt = PrepareStatement("SELECT * FROM AssetData WHERE Id = ? AND Version = ?;");
		CHECK_STMT(checkStmt)
		checkStmt.Bind(1, hash);
		checkStmt.Bind(2, version);
		int checkStmtRes = checkStmt.Step();
		if (checkStmtRes == SQLITE_ROW) {
			Statement updateStmt = PrepareStatement("UPDATE AssetData SET DataHash WHERE Id = ?;");
			CHECK_STMT(updateStmt)
			if (updateStmt.Step() != SQLITE_DONE) {
				Out("Failed to attach data to asset id {} because updating the hash {} failed. Message: \"{}\"", id, hash, GetLastErrorMsg());
				return SqlDb::Response::Failed;
			}
			return SqlDb::Response::Success;
		} else if (checkStmtRes != SQLITE_DONE) {
			Out("Failed to attach data to asset id {} because checking the hash {} failed. Message: \"{}\"", id, hash, GetLastErrorMsg());
			return SqlDb::Response::Failed;
		}
	}

	// if version is set to 0 or lower, this means that the guy who ran the function wants to create a newer version instead of overwriting an existing one.
	// so get the maximum version that is in the database and increment it by 1 to get a newer version.
	if (version <= 0) {
		Statement highestVerStmt = PrepareStatement("SELECT MAX(Version) FROM AssetData WHERE Id = ?;");
		CHECK_STMT(highestVerStmt)
		highestVerStmt.Bind(1, id);
		if (highestVerStmt.Step() == SQLITE_ROW) {
			version = highestVerStmt.GetIntFromColumnIndex(0) + 1;
		} else {
			Out("Failed to attach data to asset id {} because the latest version could not be retrieved.", id);
			return SqlDb::Response::Failed;
		}
	}

	// if an entry does not exist in database, just insert a new one
	Statement stmt = PrepareStatement("INSERT INTO AssetData (Id, Version, DataHash) VALUES (?, ?, ?);");
	CHECK_STMT(stmt)
	stmt.Bind(1, id);
	stmt.Bind(2, version);
	stmt.Bind(3, hash);
	if (stmt.Step() != SQLITE_DONE) {
		Out("Failed to attach data to asset id {} because inserting the hash {} failed. Message: \"{}\"", id, hash, GetLastErrorMsg());
		return SqlDb::Response::Failed;
	}
	return SqlDb::Response::Success;
}

SqlDb::Response EmuDb::DetachBlobHashFromAsset(int64_t id, int version, const std::string &hash) {
	if (Fail()) return SqlDb::Response::DatabaseFailed;

	// version > 0 detaches that specific version; version <= 0 detaches every version of this
	// asset that points at the given hash.
	std::string stmtStr = "DELETE FROM AssetData WHERE Id = ? AND DataHash = ?";
	if (version > 0) stmtStr += " AND Version = ?;";
	else stmtStr += ";";

	Statement stmt = PrepareStatement(stmtStr);
	CHECK_STMT(stmt)
	stmt.Bind(1, id);
	stmt.Bind(2, hash);
	if (version > 0) stmt.Bind(3, version);

	if (stmt.Step() != SQLITE_DONE) {
		Out("Failed to detach hash {} from asset id {}. Message: \"{}\"", hash, id, GetLastErrorMsg());
		return SqlDb::Response::Failed;
	}
	if (sqlite3_changes(mDb) == 0)
		return SqlDb::Response::NotFound;

	GarbageCollectBlobIfOrphaned(hash);
	MarkDirty();
	return SqlDb::Response::Success;
}

SqlDb::Response EmuDb::AttachThumbnailDataToAsset(int64_t id, const std::vector<unsigned char> &data) {
	if (Fail()) return SqlDb::Response::DatabaseFailed;

	std::string hashStr;
	SqlDb::Response res = AddBlob(data, &hashStr);
	if (res != SqlDb::Response::Success && res != SqlDb::Response::DidNothing) {
		Out("Failed to attach thumbnail to asset id {} because the blob could not be added", id);
		return SqlDb::Response::Failed;
	}

	// Attach to the latest data version so RetrieveImageData (which reads the newest version) finds it.
	Statement stmt = PrepareStatement(
		"UPDATE AssetData SET AutogeneratedThumbnailHash = ? "
		"WHERE Id = ? AND Version = (SELECT MAX(Version) FROM AssetData WHERE Id = ?);");
	CHECK_STMT(stmt)
	stmt.Bind(1, hashStr);
	stmt.Bind(2, id);
	stmt.Bind(3, id);
	if (stmt.Step() != SQLITE_DONE) {
		Out("Failed to attach thumbnail to asset id {}. Message: \"{}\"", id, GetLastErrorMsg());
		return SqlDb::Response::Failed;
	}

	// The UPDATE matches nothing when the asset has no data version yet (a metadata-only backup, or
	// the asset binary couldn't be downloaded). Create a thumbnail-only version so previews still
	// resolve through RetrieveImageData's AutogeneratedThumbnailHash lookup.
	if (sqlite3_changes(mDb) == 0) {
		Statement ins = PrepareStatement(
			"INSERT INTO AssetData (Id, Version, AutogeneratedThumbnailHash) VALUES (?, 1, ?);");
		CHECK_STMT(ins)
		ins.Bind(1, id);
		ins.Bind(2, hashStr);
		if (ins.Step() != SQLITE_DONE) {
			Out("Failed to create thumbnail-only data row for asset id {}. Message: \"{}\"", id, GetLastErrorMsg());
			return SqlDb::Response::Failed;
		}
	}
	return SqlDb::Response::Success;
}

SqlDb::Response EmuDb::UpsertAuxAssetRow(const std::string &table, int64_t id, const SqlRow &row) {
	if (Fail()) return SqlDb::Response::DatabaseFailed;

	std::string columns = "Id";
	std::string placeholders = "?";
	std::string updates;
	SqlRow fields;
	for (const auto &col : row) {
		if (col.first == "Id") continue; // Id always comes from the id parameter
		fields.push_back(col);
		columns += ", " + col.first;
		placeholders += ", ?";
		if (!updates.empty()) updates += ", ";
		updates += col.first + " = excluded." + col.first;
	}

	// Nothing but the key to write: just make sure the record exists.
	if (fields.empty()) {
		Statement stmt = PrepareStatement(std::format("INSERT OR IGNORE INTO {} (Id) VALUES (?);", table));
		CHECK_STMT(stmt)
		stmt.Bind(1, id);
		if (stmt.Step() != SQLITE_DONE) return SqlDb::Response::Failed;
		MarkDirty();
		return SqlDb::Response::Success;
	}

	// UPSERT: insert a fresh row, or fold the new values into the existing one (keyed by Id).
	std::string stmtStr = std::format(
		"INSERT INTO {} ({}) VALUES ({}) ON CONFLICT(Id) DO UPDATE SET {};",
		table, columns, placeholders, updates);

	Statement stmt = PrepareStatement(stmtStr);
	CHECK_STMT(stmt)
	stmt.Bind(1, id);
	for (size_t i = 0; i < fields.size(); i++)
		stmt.Bind(static_cast<int>(i) + 2, fields.at(i).second);

	switch (stmt.Step()) {
	default: return SqlDb::Response::Failed;
	case SQLITE_DONE: MarkDirty(); return SqlDb::Response::Success;
	case SQLITE_BUSY: return SqlDb::Response::Busy;
	case SQLITE_MISUSE: return SqlDb::Response::Misuse;
	case SQLITE_CONSTRAINT: return SqlDb::Response::ConstraintViolation;
	}
}

SqlDb::Response EmuDb::DetachAuxAssetRow(const std::string &table, int64_t id, const SqlRow &row) {
	if (Fail()) return SqlDb::Response::DatabaseFailed;

	// An empty row removes the whole auxiliary record; a non-empty row instead blanks just the
	// named columns (leaving the record and its other fields intact).
	std::string sets;
	for (const auto &col : row) {
		if (col.first == "Id") continue;
		if (!sets.empty()) sets += ", ";
		sets += col.first + " = NULL";
	}

	std::string stmtStr = sets.empty()
		? std::format("DELETE FROM {} WHERE Id = ?;", table)
		: std::format("UPDATE {} SET {} WHERE Id = ?;", table, sets);

	Statement stmt = PrepareStatement(stmtStr);
	CHECK_STMT(stmt)
	stmt.Bind(1, id);

	switch (stmt.Step()) {
	default: return SqlDb::Response::Failed;
	case SQLITE_DONE:
		MarkDirty();
		return sqlite3_changes(mDb) > 0 ? SqlDb::Response::Success : SqlDb::Response::NotFound;
	case SQLITE_BUSY: return SqlDb::Response::Busy;
	case SQLITE_MISUSE: return SqlDb::Response::Misuse;
	}
}

SqlDb::Response EmuDb::AttachHistoricalDataToAsset(int64_t id, SqlRow row) {
	return UpsertAuxAssetRow("AssetHistorical", id, row);
}

SqlDb::Response EmuDb::DetachHistoricalDataFromAsset(int64_t id, SqlRow row) {
	return DetachAuxAssetRow("AssetHistorical", id, row);
}

SqlDb::Response EmuDb::AttachMicrotransactionDataToAsset(int64_t id, SqlRow row) {
	return UpsertAuxAssetRow("AssetMicrotransaction", id, row);
}

SqlDb::Response EmuDb::DetachMicrotransactionDataFromAsset(int64_t id, SqlRow row) {
	return DetachAuxAssetRow("AssetMicrotransaction", id, row);
}

SqlDb::Response EmuDb::AddThumbnailToPlace(int64_t id, int64_t imageId) {
	if (Fail()) return SqlDb::Response::DatabaseFailed;

	// (Id, Thumbnail) is the composite key (v14); OR IGNORE makes re-adding the same thumbnail a
	// no-op. A bad imageId trips the Thumbnail->Asset(Id) foreign key (FKs ignore OR IGNORE).
	Statement stmt = PrepareStatement("INSERT OR IGNORE INTO AssetPlaceThumbnail (Id, Thumbnail) VALUES (?, ?);");
	CHECK_STMT(stmt)
	stmt.Bind(1, id);
	stmt.Bind(2, imageId);

	switch (stmt.Step()) {
	default: return SqlDb::Response::Failed;
	case SQLITE_DONE:
		MarkDirty();
		return sqlite3_changes(mDb) > 0 ? SqlDb::Response::Success : SqlDb::Response::DidNothing;
	case SQLITE_BUSY: return SqlDb::Response::Busy;
	case SQLITE_MISUSE: return SqlDb::Response::Misuse;
	case SQLITE_CONSTRAINT: return SqlDb::Response::ConstraintViolation;
	}
}

SqlDb::Response EmuDb::RemoveThumbnailFromPlace(int64_t id, int64_t imageId) {
	if (Fail()) return SqlDb::Response::DatabaseFailed;

	Statement stmt = PrepareStatement("DELETE FROM AssetPlaceThumbnail WHERE Id = ? AND Thumbnail = ?;");
	CHECK_STMT(stmt)
	stmt.Bind(1, id);
	stmt.Bind(2, imageId);

	switch (stmt.Step()) {
	default: return SqlDb::Response::Failed;
	case SQLITE_DONE:
		MarkDirty();
		return sqlite3_changes(mDb) > 0 ? SqlDb::Response::Success : SqlDb::Response::NotFound;
	case SQLITE_BUSY: return SqlDb::Response::Busy;
	case SQLITE_MISUSE: return SqlDb::Response::Misuse;
	}
}

SqlDb::Response EmuDb::RenderThumbnailForAsset(int64_t id, int version) {
	if (Fail()) return SqlDb::Response::DatabaseFailed;

	if (!DoesItemExist(ItemType::Asset, id)) {
		Out("Cannot render thumbnail for asset id {}: asset does not exist", id);
		return SqlDb::Response::NotFound;
	}

	Out("RenderThumbnailForAsset(id={}, version={}) is not wired up yet: it needs the background "
		"RCCService render pipeline (see RccServiceManager). No thumbnail was produced.", id, version);
	return SqlDb::Response::Failed;
}

SqlDb::Response EmuDb::RetrieveAssetData(int64_t id, int version, std::vector<unsigned char> *dataOutput, std::string *hashOutput) {
    if (Fail()) return SqlDb::Response::DatabaseFailed;

    Statement checkAssetExistsStmt = PrepareStatement("SELECT Id FROM Asset WHERE Id = ?;");
    CHECK_STMT(checkAssetExistsStmt);
    checkAssetExistsStmt.Bind(1, id);
    if (checkAssetExistsStmt.Step() != SQLITE_ROW)
        return SqlDb::Response::NotFound;

    std::string stmtStr = "SELECT DataHash FROM AssetData WHERE Id = ?";
    if (version > 0)
        stmtStr += " AND Version = ?;";
    else
        stmtStr += " ORDER BY Version DESC LIMIT 1;";

    Statement getHashStmt = PrepareStatement(stmtStr);
    CHECK_STMT(getHashStmt)
    getHashStmt.Bind(1, id);
    if (version > 0)
        getHashStmt.Bind(2, version);

    int res = getHashStmt.Step();
    if (res != SQLITE_ROW && res != SQLITE_DONE)
        return SqlDb::Response::Failed;
    if (res == SQLITE_ROW) {
        std::string hash = getHashStmt.GetStringFromColumnIndex(0);
        if (hashOutput != nullptr)
            *hashOutput = hash;

        Statement blobStmt = PrepareStatement("SELECT Blob FROM BlobStorage WHERE Hash = ?");
        CHECK_STMT(getHashStmt)
        blobStmt.Bind(1, hash);

        int blobStmtRes = blobStmt.Step();
        if (blobStmtRes != SQLITE_ROW && blobStmtRes != SQLITE_DONE)
            return SqlDb::Response::Failed;
        if (blobStmtRes == SQLITE_ROW) {
			std::vector<unsigned char> data = blobStmt.GetBlobFromColumnIndex(0);

			if (IsZstdCompressed(data)) {
				unsigned long long decompSize = ZSTD_getFrameContentSize(data.data(), data.size());
				if (decompSize == ZSTD_CONTENTSIZE_ERROR || decompSize == ZSTD_CONTENTSIZE_UNKNOWN) {
					return SqlDb::Response::BlobDecompressionFailed;
				}

				std::vector<unsigned char> decompressed(decompSize);
				size_t result = ZSTD_decompress(
					decompressed.data(), decompSize,
					data.data(), data.size()
				);
				if (ZSTD_isError(result)) {
					return SqlDb::Response::BlobDecompressionFailed;
				}
				data = std::move(decompressed);
			}

            if (dataOutput != nullptr)
                *dataOutput = std::move(data);
            return SqlDb::Response::Success;
        }
    }
    return SqlDb::Response::MissingBlob;
}

std::optional<int64_t> EmuDb::GetUniverseIdForPlace(int64_t placeId) {
	if (Fail()) return std::nullopt;

	// A place can belong to a universe explicitly, via the UniversePlace junction table...
	{
		Statement stmt = PrepareStatement("SELECT Id FROM UniversePlace WHERE PlaceId = ? LIMIT 1;");
		if (!stmt.Fail()) {
			stmt.Bind(1, placeId);
			if (stmt.Step() == SQLITE_ROW)
				return stmt.GetInt64FromColumnIndex(0);
		}
	}

	// ...or by being the start (root) place of a universe.
	{
		Statement stmt = PrepareStatement("SELECT Id FROM Universe WHERE StartPlaceId = ? LIMIT 1;");
		if (!stmt.Fail()) {
			stmt.Bind(1, placeId);
			if (stmt.Step() == SQLITE_ROW)
				return stmt.GetInt64FromColumnIndex(0);
		}
	}

	return std::nullopt;
}

std::optional<int64_t> EmuDb::GetStartPlaceIdForUniverse(int64_t universeId) {
	if (Fail()) return std::nullopt;

	Statement stmt = PrepareStatement("SELECT StartPlaceId FROM Universe WHERE Id = ?;");
	if (stmt.Fail()) return std::nullopt;
	stmt.Bind(1, universeId);
	if (stmt.Step() == SQLITE_ROW && !stmt.IsColumnIndexNull(0))
		return stmt.GetInt64FromColumnIndex(0);
	return std::nullopt;
}

std::optional<std::string> EmuDb::GetItemName(ItemType type, int64_t id) {
	if (Fail()) return std::nullopt;

	Statement stmt = PrepareStatement(std::format("SELECT Name FROM {} WHERE Id = ?;", GetTableNameFromItemType(type)));
	if (stmt.Fail()) return std::nullopt;
	stmt.Bind(1, id);
	if (stmt.Step() == SQLITE_ROW && !stmt.IsColumnIndexNull(0))
		return stmt.GetStringFromColumnIndex(0);
	return std::nullopt;
}

std::optional<int64_t> EmuDb::GetCreatorUserId(ItemType type, int64_t id) {
	if (Fail()) return std::nullopt;

	Statement stmt = PrepareStatement(std::format("SELECT UserId FROM {} WHERE Id = ?;", GetTableNameFromItemType(type)));
	if (stmt.Fail()) return std::nullopt;
	stmt.Bind(1, id);
	if (stmt.Step() == SQLITE_ROW && !stmt.IsColumnIndexNull(0))
		return stmt.GetInt64FromColumnIndex(0);
	return std::nullopt;
}

std::vector<int64_t> EmuDb::SearchAssetIds(Roblox::AssetType type, const std::string &keyword, int limit, int offset) {
	std::vector<int64_t> ids;
	if (Fail()) return ids;
	if (limit <= 0) limit = 30;
	if (offset < 0) offset = 0;

	std::string sql = "SELECT Id FROM Asset WHERE 1=1";
	if (type != Roblox::AssetType::None) sql += " AND Type = ?";
	if (!keyword.empty()) sql += " AND Name LIKE ?";
	sql += " ORDER BY Id DESC LIMIT ? OFFSET ?;";

	Statement stmt = PrepareStatement(sql);
	if (stmt.Fail()) return ids;

	int idx = 1;
	if (type != Roblox::AssetType::None) stmt.Bind(idx++, static_cast<int>(type));
	if (!keyword.empty()) stmt.Bind(idx++, "%" + keyword + "%");
	stmt.Bind(idx++, limit);
	stmt.Bind(idx++, offset);

	while (stmt.Step() == SQLITE_ROW)
		ids.push_back(stmt.GetInt64FromColumnIndex(0));
	return ids;
}

std::optional<EmuDb::AssetSummary> EmuDb::GetAssetSummary(int64_t id) {
	if (Fail()) return std::nullopt;

	Statement stmt = PrepareStatement("SELECT Id, Name, Description, Type, UserId, GroupId, Created, Updated FROM Asset WHERE Id = ?;");
	if (stmt.Fail()) return std::nullopt;
	stmt.Bind(1, id);
	if (stmt.Step() != SQLITE_ROW)
		return std::nullopt;

	AssetSummary summary;
	summary.Id = stmt.GetInt64FromColumnIndex(0);
	summary.Name = stmt.GetStringFromColumnIndex(1);
	summary.Description = stmt.IsColumnIndexNull(2) ? "" : stmt.GetStringFromColumnIndex(2);
	summary.Type = stmt.GetIntFromColumnIndex(3);
	if (!stmt.IsColumnIndexNull(4)) summary.UserId = stmt.GetInt64FromColumnIndex(4);
	if (!stmt.IsColumnIndexNull(5)) summary.GroupId = stmt.GetInt64FromColumnIndex(5);
	if (!stmt.IsColumnIndexNull(6)) summary.Created = stmt.GetInt64FromColumnIndex(6);
	if (!stmt.IsColumnIndexNull(7)) summary.Updated = stmt.GetInt64FromColumnIndex(7);
	return summary;
}

SqlDb::Response EmuDb::AddAssetLink(const std::string &table, int64_t ownerId, int64_t assetId) {
	if (Fail()) return SqlDb::Response::DatabaseFailed;

	// BundleAsset, OutfitItem, and UserCharacterItem all share the (Id, AssetId) shape with a
	// composite primary key, so re-linking is a harmless no-op under OR IGNORE. A bad assetId (or
	// ownerId) trips the table's foreign keys, which OR IGNORE does not suppress.
	Statement stmt = PrepareStatement(std::format("INSERT OR IGNORE INTO {} (Id, AssetId) VALUES (?, ?);", table));
	CHECK_STMT(stmt)
	stmt.Bind(1, ownerId);
	stmt.Bind(2, assetId);

	switch (stmt.Step()) {
	default: return SqlDb::Response::Failed;
	case SQLITE_DONE:
		MarkDirty();
		return sqlite3_changes(mDb) > 0 ? SqlDb::Response::Success : SqlDb::Response::DidNothing;
	case SQLITE_BUSY: return SqlDb::Response::Busy;
	case SQLITE_MISUSE: return SqlDb::Response::Misuse;
	case SQLITE_CONSTRAINT: return SqlDb::Response::ConstraintViolation;
	}
}

SqlDb::Response EmuDb::RemoveAssetLink(const std::string &table, int64_t ownerId, int64_t assetId) {
	if (Fail()) return SqlDb::Response::DatabaseFailed;

	Statement stmt = PrepareStatement(std::format("DELETE FROM {} WHERE Id = ? AND AssetId = ?;", table));
	CHECK_STMT(stmt)
	stmt.Bind(1, ownerId);
	stmt.Bind(2, assetId);

	switch (stmt.Step()) {
	default: return SqlDb::Response::Failed;
	case SQLITE_DONE:
		MarkDirty();
		return sqlite3_changes(mDb) > 0 ? SqlDb::Response::Success : SqlDb::Response::NotFound;
	case SQLITE_BUSY: return SqlDb::Response::Busy;
	case SQLITE_MISUSE: return SqlDb::Response::Misuse;
	}
}

SqlDb::Response EmuDb::AddAssetToBundle(int64_t bundleId, int64_t assetId) {
	return AddAssetLink("BundleAsset", bundleId, assetId);
}

SqlDb::Response EmuDb::RemoveAssetFromBundle(int64_t bundleId, int64_t assetId) {
	return RemoveAssetLink("BundleAsset", bundleId, assetId);
}

SqlDb::Response EmuDb::AddAssetToOutfit(int64_t outfitId, int64_t assetId) {
	return AddAssetLink("OutfitItem", outfitId, assetId);
}

SqlDb::Response EmuDb::RemoveAssetFromOutfit(int64_t outfitId, int64_t assetId) {
	return RemoveAssetLink("OutfitItem", outfitId, assetId);
}

SqlDb::Response EmuDb::AddAssetToUserCharacter(int64_t userId, int64_t assetId) {
	return AddAssetLink("UserCharacterItem", userId, assetId);
}

SqlDb::Response EmuDb::RemoveAssetFromUserCharacter(int64_t userId, int64_t assetId) {
	return RemoveAssetLink("UserCharacterItem", userId, assetId);
}

std::vector<unsigned char> EmuDb::RetrieveImageData(NoobWarrior::ItemType itemType, int64_t id) {
    auto faily = [&](const std::string &reason) {
        Out(std::format("Failed to retrieve image data for ID {}: {}", id, reason));
        return std::vector<unsigned char>(g_icon_content_deleted, g_icon_content_deleted + g_icon_content_deleted_size);
    };

    if (Fail()) return faily("Database initialization failed");

    // NB: must be 64-bit. Modern Roblox asset/universe IDs exceed 2^31, so a plain int truncates the
    // id, the row lookup misses, and every large-id item falls back to the placeholder image.
    int64_t currentId = id;
    NoobWarrior::ItemType currentType = itemType;

    for (int i = 0; i < 10; ++i) {
		if (currentType == NoobWarrior::ItemType::Universe) {
			Statement typeStmt = PrepareStatement("SELECT StartPlaceId FROM Universe WHERE Id = ?;");
			if (typeStmt.Fail()) return faily("Failed to retrieve asset type");
            typeStmt.Bind(1, currentId);

			if (typeStmt.Step() != SQLITE_ROW)
                break;

			int64_t startPlaceId = typeStmt.GetInt64FromColumnIndex(0);
			if (startPlaceId == 0)
				break;
			
			return RetrieveImageData(NoobWarrior::ItemType::Asset, startPlaceId);
		}
        if (currentType == NoobWarrior::ItemType::Asset) {
            Statement typeStmt = PrepareStatement("SELECT Type, ImageId FROM Asset WHERE Id = ?;");
            if (typeStmt.Fail()) return faily("Failed to retrieve asset type");
            typeStmt.Bind(1, currentId);

            if (typeStmt.Step() != SQLITE_ROW)
                return faily("Asset not found");

            int type = typeStmt.GetIntFromColumnIndex(0);
            int64_t imageId = typeStmt.GetInt64FromColumnIndex(1);

            if (type != static_cast<int>(Roblox::AssetType::Image) && imageId != 0)
                return RetrieveImageData(NoobWarrior::ItemType::Asset, imageId);

            // An Image asset's own data IS the picture, so use it when present. If the binary was
            // never downloaded (e.g. assetdelivery requires auth/ownership), fall through to the
            // autogenerated thumbnail below instead of giving up.
            if (type == static_cast<int>(Roblox::AssetType::Image)) {
                Statement hashStmt = PrepareStatement("SELECT DataHash FROM AssetData WHERE Id = ? AND DataHash IS NOT NULL ORDER BY Version DESC LIMIT 1;");
                if (!hashStmt.Fail()) {
                    hashStmt.Bind(1, currentId);
                    if (hashStmt.Step() == SQLITE_ROW) {
                        std::string hash = hashStmt.GetStringFromColumnIndex(0);
                        Statement blobStmt = PrepareStatement("SELECT Blob FROM BlobStorage WHERE Hash = ?;");
                        if (!hash.empty() && !blobStmt.Fail()) {
                            blobStmt.Bind(1, hash);
                            if (blobStmt.Step() == SQLITE_ROW) {
                                std::vector<unsigned char> blob = blobStmt.GetBlobFromColumnIndex(0);
                                if (!blob.empty()) {
                                    if (IsZstdCompressed(blob)) {
                                        unsigned long long decompSize = ZSTD_getFrameContentSize(blob.data(), blob.size());
                                        if (decompSize != ZSTD_CONTENTSIZE_ERROR && decompSize != ZSTD_CONTENTSIZE_UNKNOWN) {
                                            std::vector<unsigned char> decompressed(decompSize);
                                            size_t result = ZSTD_decompress(decompressed.data(), decompSize, blob.data(), blob.size());
                                            if (!ZSTD_isError(result)) {
                                                GunzipIfNeeded(decompressed); // older backups stored gzip'd asset bytes
                                                return decompressed;
                                            }
                                        }
                                    } else {
                                        GunzipIfNeeded(blob);
                                        return blob;
                                    }
                                }
                            }
                        }
                    }
                }
                // No usable raw image data — fall through to the autogenerated thumbnail.
            }

            {
                Statement thumbStmt = PrepareStatement("SELECT AutogeneratedThumbnailHash FROM AssetData WHERE Id = ? AND AutogeneratedThumbnailHash IS NOT NULL ORDER BY Version DESC LIMIT 1;");
                if (!thumbStmt.Fail()) {
                    thumbStmt.Bind(1, currentId);
                    if (thumbStmt.Step() == SQLITE_ROW) {
                    std::string thumbHash = thumbStmt.GetStringFromColumnIndex(0);
                    if (!thumbHash.empty()) {
                        Statement blobStmt = PrepareStatement("SELECT Blob FROM BlobStorage WHERE Hash = ?;");
                        if (!blobStmt.Fail()) {
                            blobStmt.Bind(1, thumbHash);
                            if (blobStmt.Step() == SQLITE_ROW) {
                                std::vector<unsigned char> blob = blobStmt.GetBlobFromColumnIndex(0);
                                if (!blob.empty()) {
                                    if (IsZstdCompressed(blob)) {
                                        unsigned long long decompSize = ZSTD_getFrameContentSize(blob.data(), blob.size());
                                        if (decompSize != ZSTD_CONTENTSIZE_ERROR && decompSize != ZSTD_CONTENTSIZE_UNKNOWN) {
                                            std::vector<unsigned char> decompressed(decompSize);
                                            size_t result = ZSTD_decompress(decompressed.data(), decompSize, blob.data(), blob.size());
                                            if (!ZSTD_isError(result)) {
                                                GunzipIfNeeded(decompressed); // older backups stored gzip'd asset bytes
                                                return decompressed;
                                            }
                                        }
                                    } else {
                                        GunzipIfNeeded(blob);
                                        return blob;
                                    }
                                }
                            }
                        }
                    }
                    }
                }
            }

            return RetrieveAssetTypeImageData(static_cast<Roblox::AssetType>(type));
        }

        // Follow ImageId redirect into Asset table
        std::string tableName = GetTableNameFromItemType(currentType);
        Statement imgIdStmt = PrepareStatement(std::format("SELECT ImageId FROM {} WHERE Id = ?;", tableName));
        if (imgIdStmt.Fail()) return faily("Failed to prepare ImageId statement");
        imgIdStmt.Bind(1, currentId);

        if (imgIdStmt.Step() != SQLITE_ROW)
            break;

        currentId = imgIdStmt.GetInt64FromColumnIndex(0);
        currentType = NoobWarrior::ItemType::Asset;

        if (i == 9)
            Out(std::format("ImageId redirect loop limit reached for ID {}", id));
    }

    return std::vector<unsigned char>(g_icon_content_deleted, g_icon_content_deleted + g_icon_content_deleted_size);
}

std::vector<unsigned char> EmuDb::RetrieveBlobFromTableName(int64_t id, const std::string &tableName,
	const std::string &columnName) {
	if (Fail()) return {};

	std::string stmtStr = std::format("SELECT * FROM {} WHERE Id = ? ORDER BY Snapshot DESC LIMIT 1;", tableName);

	Statement stmt(this, stmtStr);
	stmt.Bind(1, id);

	if (stmt.Step() == SQLITE_ROW) {
		const auto data = GetValueFromColumnName<std::vector<unsigned char>>(stmt.Get(), columnName);
		return data;
	}
    return {};
}