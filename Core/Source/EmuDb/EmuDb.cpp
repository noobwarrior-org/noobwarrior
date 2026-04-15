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
#include <cstdio>

#include "../algorithm/base64.h"

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

using namespace NoobWarrior;

std::vector<unsigned char> RetrieveAssetTypeImageData(Roblox::AssetType type) {
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
	}
	return imgData;
}

EmuDb::EmuDb(const std::string &path, bool autocommit) :
	SqlDb(path, "EmuDb"),
	mAutoCommit(autocommit),
	mDirty(false),
	mAssetRepository(this),
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

	int prevRowId;
	std::string prevVersion;
	sqlite3_int64 prevTimestamp;

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

			if (rowId > prevRowId && prevVersion > version) {
				mMigrationFailMsg = std::format("Failed to verify integrity of migration: the newer version {} has a lower number than previous version {}. Did the developer order the versions wrong?", version, prevVersion);
				Out(mMigrationFailMsg);
				return false;
			}

			prevRowId = rowId;
			prevVersion = version;
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

	Statement stmt = PrepareStatement("INSERT INTO BlobStorage (Hash, Blob) VALUES (?, ?);");
	CHECK_STMT(stmt)
	stmt.Bind(1, hashStr);
	stmt.Bind(2, data);

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
    if (!file.is_open()) {
        return SqlDb::Response::CantOpen;
    }

    uintmax_t fileSize = 0;
    try {
        fileSize = std::filesystem::file_size(path);
        if (fileSize >= 2147483648) {
            return SqlDb::Response::BlobTooLarge;
        }
    } catch (std::filesystem::filesystem_error &e) {
        return SqlDb::Response::CantOpen;
    }

    SHA256_CTX ctx;
    SHA256_Init(&ctx);

    std::vector<char> buf(1024);
    while (file.read(buf.data(), 1024) || file.gcount() > 0) {
        int n = static_cast<int>(file.gcount());
        SHA256_Update(&ctx, buf.data(), n);
    }

    // seek back to the beginning because we're going to be reading this file again
    file.clear();
    file.seekg(0, std::ios::beg);

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &ctx);

    // seek back to the beginning because we're going to be reading this file again
    file.clear();
    file.seekg(0, std::ios::beg);

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

    Statement insertStmt = PrepareStatement("INSERT OR IGNORE INTO BlobStorage (Hash, Blob) VALUES (?, ZEROBLOB(?));");
    CHECK_STMT(insertStmt)
    insertStmt.Bind(1, hashStr);
    insertStmt.Bind(2, (sqlite3_int64)fileSize);
    switch (insertStmt.Step()) {
    case SQLITE_DONE: break;
    case SQLITE_BUSY: return SqlDb::Response::Busy;
    case SQLITE_MISUSE: return SqlDb::Response::Misuse;
    case SQLITE_CONSTRAINT: return SqlDb::Response::ConstraintViolation;
    default: return SqlDb::Response::Failed;
    }

    sqlite3_blob *blob = nullptr;
    sqlite3_blob_open(
        mDb,
        "main",
        "BlobStorage",
        "Blob",
        sqlite3_last_insert_rowid(mDb),
        1,
        &blob
    );

    std::vector<char> buf2(64 * 1024);
    int offset = 0;
    while (file.read(buf2.data(), 64 * 1024) || file.gcount() > 0) {
        int n = static_cast<int>(file.gcount());
        sqlite3_blob_write(blob, buf2.data(), n, offset);
        offset += n;
    }

    sqlite3_blob_close(blob);
    if (hashOutput != nullptr)
        *hashOutput = hashStr;
    return SqlDb::Response::Success;
}

SqlDb::Response EmuDb::AddItem(ItemType type, SqlRow row) {
	if (Fail()) return SqlDb::Response::DatabaseFailed;

	std::string tableName = GetTableNameFromItemType(type);
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

SqlDb::Response EmuDb::DeleteItem(ItemType type, int64_t id) {
	if (Fail()) return SqlDb::Response::DatabaseFailed;

	Statement stmt = PrepareStatement("DELETE FROM " + GetTableNameFromItemType(type) + " WHERE Id = ?;");
	stmt.Bind(1, id);
	switch (stmt.Step()) {
	default:
		return SqlDb::Response::Failed;
	case SQLITE_DONE:
		return SqlDb::Response::Success;
	case SQLITE_BUSY:
		return SqlDb::Response::Busy;
	case SQLITE_MISUSE:
		return SqlDb::Response::Misuse;
	}
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

SqlDb::Response EmuDb::DetachDataFromAsset(int64_t id, int version) {
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
}

SqlDb::Response EmuDb::AttachHistoricalDataToAsset(int64_t id, SqlRow row) {
}

SqlDb::Response EmuDb::DetachHistoricalDataFromAsset(int64_t id, SqlRow row) {
}

SqlDb::Response EmuDb::AttachMicrotransactionDataToAsset(int64_t id, SqlRow row) {
}

SqlDb::Response EmuDb::DetachMicrotransactionDataFromAsset(int64_t id, SqlRow row) {
}

SqlDb::Response EmuDb::AddThumbnailToPlace(int64_t id, int64_t imageId) {
}

SqlDb::Response EmuDb::RemoveThumbnailFromPlace(int64_t id, int64_t imageId) {
}

SqlDb::Response EmuDb::RenderThumbnailForAsset(int64_t id, int version) {
}

SqlDb::Response EmuDb::RetrieveAssetData(int64_t id, int version, std::vector<unsigned char> *dataOutput) {
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
        Statement blobStmt = PrepareStatement("SELECT Blob FROM BlobStorage WHERE Hash = ?");
        CHECK_STMT(getHashStmt)
        blobStmt.Bind(1, hash);

        int blobStmtRes = blobStmt.Step();
        if (blobStmtRes != SQLITE_ROW && blobStmtRes != SQLITE_DONE)
            return SqlDb::Response::Failed;
        if (blobStmtRes == SQLITE_ROW) {
            if (dataOutput != nullptr)
                *dataOutput = blobStmt.GetBlobFromColumnIndex(0);
            return SqlDb::Response::Success;
        }
    }
    return SqlDb::Response::MissingBlob;
}

SqlDb::Response EmuDb::AddAssetToBundle(int64_t bundleId, int64_t assetId) {
}

SqlDb::Response EmuDb::RemoveAssetFromBundle(int64_t bundleId, int64_t assetId) {
}

SqlDb::Response EmuDb::AddAssetToOutfit(int64_t outfitId, int64_t assetId) {
}

SqlDb::Response EmuDb::RemoveAssetFromOutfit(int64_t outfitId, int64_t assetId) {
}

SqlDb::Response EmuDb::AddAssetToUserCharacter(int64_t userId, int64_t assetId) {
}

SqlDb::Response EmuDb::RemoveAssetFromUserCharacter(int64_t userId, int64_t assetId) {
}

AssetRepository* EmuDb::GetAssetRepository() {
	return &mAssetRepository;
}

std::vector<unsigned char> EmuDb::RetrieveImageData(const std::string &tableName, int64_t id) {
	std::vector<unsigned char> imgData;
#define FAIL(...) \
	Out(std::format("Failed to retrieve image data for ID {} from table {}: ", id, tableName) + __VA_ARGS__); \
	imgData.assign(g_icon_content_deleted, g_icon_content_deleted + g_icon_content_deleted_size); \
	return imgData;

	if (Fail()) {
		FAIL("Database initialization failed")
	}

    int currentId = id;
    std::string currentTableName = tableName;

    // Loop to handle ImageId redirection without recursion
    for (int i = 0; i < 10; ++i) { // Limit iterations to prevent infinite loops in case of bad data
        if (currentTableName.compare("Asset") == 0) {
            // If we are an image asset, we need to get the data from ourselves directly
            int type = 0;

            Statement typeStmt = PrepareStatement(std::format("SELECT Type FROM {} WHERE Id = ?;", currentTableName));
            if (typeStmt.Fail()) {
                FAIL("Failed to retrieve asset type for ID {}", currentId)
            }
            typeStmt.Bind(1, currentId);

            if (typeStmt.Step() == SQLITE_ROW) {
                type = typeStmt.GetIntFromColumnIndex(0);
            }

            if (type == static_cast<int>(Roblox::AssetType::Image)) {
                Statement hashStmt = PrepareStatement("SELECT DataHash FROM AssetData WHERE Id = ? ORDER BY Version DESC LIMIT 1;");
                if (hashStmt.Fail()) {
                    FAIL("Failed to prepare statement in order to retrieve image hash")
                }
                hashStmt.Bind(1, currentId);

                if (hashStmt.Step() == SQLITE_ROW) {
                    std::string hash = hashStmt.GetStringFromColumnIndex(0);

                    Statement imgDataStmt = PrepareStatement("SELECT Blob FROM BlobStorage WHERE Hash = ?;");
                    if (imgDataStmt.Fail()) {
                        FAIL("Failed to prepare statement in order to retrieve image data")
                    }
                    imgDataStmt.Bind(1, hash);

                    if (imgDataStmt.Step() == SQLITE_ROW) {
                        std::vector<unsigned char> imgBlob = imgDataStmt.GetBlobFromColumnIndex(0);
                        if (imgBlob.size() == 0) {
                            imgData.assign(g_icon_content_deleted, g_icon_content_deleted + g_icon_content_deleted_size);
                            return imgData;
                        }
                        return imgBlob;
                    }
                }
            }
        }

        // If not an image asset or not an asset table, try to find an ImageId
        Statement imgIdStmt = PrepareStatement(std::format("SELECT ImageId FROM {} WHERE Id = ?;",
            currentTableName
        ));
        if (imgIdStmt.Fail()) {
            FAIL("Failed to prepare statement in order to retrieve image ID")
        }
        imgIdStmt.Bind(1, currentId);

        if (imgIdStmt.Step() == SQLITE_ROW) {
            int imageId = imgIdStmt.GetIntFromColumnIndex(0);
            currentId = imageId;
            currentTableName = "Asset"; // Next iteration, treat it as an Asset
        } else {
            // No ImageId found, or it was an Asset but not an Image type and no ImageId was found
            break;
        }

        if (i == 9)
            Out(std::format("ImageId redirect loop limit reached for ID {} in table {}", id, tableName));
    }

	imgData.assign(g_icon_content_deleted, g_icon_content_deleted + g_icon_content_deleted_size);
	return imgData;
#undef FAIL
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