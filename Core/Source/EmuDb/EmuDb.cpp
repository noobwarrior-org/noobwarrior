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
// File: Database.cpp
// Started by: Hattozo
// Started on: 2/17/2025
// Description: Encapsulates a SQLite database and creates tables containing Roblox assets and other kinds of data
// You can find documentation on the file format in the corresponding header file.
#include <NoobWarrior/EmuDb/EmuDb.h>
#include <NoobWarrior/SqlDb/Common.h>
#include <NoobWarrior/SqlDb/Statement.h>
#include <NoobWarrior/NoobWarrior.h>

#include <sqlite3.h>
#include <cstdio>

#include "../base64.h"
#include "schema/table/migration.sql.inc.cpp"
#include "migrations/v1.sql.inc.cpp"
#include "migrations/v2.sql.inc.cpp"
#include "migrations/v3.sql.inc.cpp"
#include "migrations/v4.sql.inc.cpp"

using namespace NoobWarrior;

EmuDb::EmuDb(const std::string &path, bool autocommit) :
	SqlDb(path, "EmuDb"),
	mAutoCommit(autocommit),
	mDirty(false),
	mAssetRepository(this)
{
	if (Fail())
		return;

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

	mFailReason = FailReason::None;
};

bool EmuDb::VerifyIntegrityOfMigration() {
	Statement stmt = PrepareStatement("SELECT * FROM Migration");
	if (stmt.Fail()) {
		// and what do you do if you're the user? NOTHING
		Out("Failed to verify integrity of migration: select statement failed. The Migration table most likely does not exist.");
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
				Out("Failed to verify integrity of migration: cannot convert version string to number!");
				return false;
			}

			if (verToStr != rowId) {
				Out("Failed to verify integrity of migration: version {} does not match row ID {}. Did the developer order the versions wrong? Is there a gap?", version, rowId);
				return false;
			}

			if (rowId > prevRowId && prevVersion > version) {
				Out("Failed to verify integrity of migration: the newer version {} has a lower number than previous version {}. Did the developer order the versions wrong?", version, prevVersion);
				return false;
			}

			prevRowId = rowId;
			prevVersion = version;
			prevTimestamp = timestamp;
		} else {
			if (step != SQLITE_DONE) {
				Out("Failed to verify integrity of migration: could not select from migration table. Maybe it doesn't exist?");
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
			Out("Failed to begin new transaction in order to perform migration");
			return false;
		}
	}

#define CREATE_TABLE(var) \
	if (!ExecStatement(var)) { \
		Out("Failed to create table from variable \"{}\"", #var); \
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
		Out("Failed to prepare select statement for Migration table");
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
			if (addToListStmt.Step() == SQLITE_DONE) { Out("Migrated to " #migration); } \
			else { Out("Failed to insert row into migraton table. What the fuck?"); return false; } \
		} else { Out("Migration to " #migration " failed."); return false; } \
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

SqlDb::Response EmuDb::SetAuthor(const std::string &author) {
	return SetMetaKeyValue("Author", author);
}

SqlDb::Response EmuDb::SetIcon(const std::vector<unsigned char> &icon) {
	return SetMetaKeyValue("Icon", base64_encode(icon.data(), icon.size()));
}

AssetRepository* EmuDb::GetAssetRepository() {
	return &mAssetRepository;
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
