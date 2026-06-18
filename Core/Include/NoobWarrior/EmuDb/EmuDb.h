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
#include <NoobWarrior/EmuDb/ItemType.h>
#include <NoobWarrior/EmuDb/ContentImages.h>
#include <NoobWarrior/EmuDb/Item/Asset.h>
#include <NoobWarrior/EmuDb/Item/Universe.h>
#include <NoobWarrior/EmuDb/Item/User.h>
#include <NoobWarrior/EmuDb/Item/Badge.h>

#include <NoobWarrior/Roblox/Api/Asset.h>
#include <NoobWarrior/Log.h>

#include <sqlite3.h>

#include <filesystem>
#include <optional>
#include <set>
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

    static std::vector<unsigned char> RetrieveAssetTypeImageData(Roblox::AssetType type);
    static bool IsZstdCompressed(const std::vector<unsigned char>& data);

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

    /* Meta functions */
    std::string GetMetaKeyValue(const std::string &key);
    std::string GetTitle();
    std::string GetDescription();
    std::string GetVersion();
    std::string GetAuthor();
    std::vector<unsigned char> GetIcon();
    CompressionType GetCompressionType();

    SqlDb::Response SetMetaKeyValue(const std::string &key, const std::string &value);
    SqlDb::Response SetTitle(const std::string &title);
    SqlDb::Response SetDescription(const std::string &desc);
    SqlDb::Response SetVersion(const std::string &ver);
    SqlDb::Response SetAuthor(const std::string &author);
    SqlDb::Response SetIcon(const std::vector<unsigned char> &icon);
    
    /* Generic item functions */
    SqlDb::Response AddBlob(const std::vector<unsigned char> &data, std::string *hashOutput = nullptr);
    SqlDb::Response AddBlob(const std::filesystem::path &path, std::string *hashOutput = nullptr);
    SqlDb::Response AddItem(ItemType type, SqlRow row);
    SqlDb::Response UpdateItem(ItemType type, int64_t id, SqlRow row);
    SqlDb::Response DeleteItem(ItemType type, int64_t id);
    bool DoesItemExist(ItemType type, int64_t id);
    
    // A deep, self-contained snapshot of a single item: its parent row, every dependent row that
    // the schema's foreign keys hang off it, and the raw content-addressed blobs those rows
    // reference. Produced by ExportItem and consumed by ImportItem to copy an item from one
    // database into another (the basis of the SDK's copy/cut/paste).
    struct ItemSnapshot {
        ItemType Type {};
        int64_t Id {0};
        struct TableData {
            std::string Table;
            SqlRows Rows;
        };
        std::vector<TableData> Tables;   // Tables[0] is always the parent item's table.
        // hash -> the exact bytes stored in BlobStorage. Kept in their stored (possibly
        // zstd-compressed) form; the hash still identifies the uncompressed data, so a copied
        // blob stays valid and resolvable in any target database regardless of its compression.
        std::vector<std::pair<std::string, std::vector<unsigned char>>> Blobs;
    };

    // Reads a complete, transferable copy of an item (its row, dependent rows, and blobs) into
    // `out`. Returns NotFound if the item isn't in this database.
    SqlDb::Response ExportItem(ItemType type, int64_t id, ItemSnapshot *out);

    // Writes a previously exported item into this database. If an item with the same id already
    // exists: overwrite=true replaces it (delete-then-insert), overwrite=false writes nothing and
    // returns ConstraintViolation. The whole import runs inside a savepoint, so a failure leaves
    // the database untouched.
    SqlDb::Response ImportItem(const ItemSnapshot &snapshot, bool overwrite);

    /* Asset functions */
    SqlDb::Response AttachDataToAsset(int64_t id, int version, const std::vector<unsigned char> &data);
    SqlDb::Response DetachDataFromAsset(int64_t id, int version);

    SqlDb::Response AttachBlobHashToAsset(int64_t id, int version, const std::string &hash);
    SqlDb::Response DetachBlobHashFromAsset(int64_t id, int version, const std::string &hash);

    // Stores an externally-rendered thumbnail (e.g. fetched from Roblox) as the asset's
    // autogenerated thumbnail, attached to its latest data version. Used by AssetEnricher so
    // grabbed assets show a preview in the SDK without polluting the item list with fake images.
    SqlDb::Response AttachThumbnailDataToAsset(int64_t id, const std::vector<unsigned char> &data);

    SqlDb::Response AttachHistoricalDataToAsset(int64_t id, SqlRow row);
    SqlDb::Response DetachHistoricalDataFromAsset(int64_t id, SqlRow row);

    SqlDb::Response AttachMicrotransactionDataToAsset(int64_t id, SqlRow row);
    SqlDb::Response DetachMicrotransactionDataFromAsset(int64_t id, SqlRow row);

    SqlDb::Response AddThumbnailToPlace(int64_t id, int64_t imageId);
    SqlDb::Response RemoveThumbnailFromPlace(int64_t id, int64_t imageId);

    SqlDb::Response RenderThumbnailForAsset(int64_t id, int version = 0);

    SqlDb::Response RetrieveAssetData(int64_t id, int version, std::vector<unsigned char> *dataOutput, std::string *hashOutput = nullptr);

    /* Universe/place functions */

    // Links/unlinks a place to a universe through the UniversePlace junction table. AddPlaceToUniverse
    // is idempotent (re-adding the same pair is a no-op).
    SqlDb::Response AddPlaceToUniverse(int64_t universeId, int64_t placeId);
    SqlDb::Response RemovePlaceFromUniverse(int64_t universeId, int64_t placeId);

    // Resolves the universe a place belongs to. A place is linked to a universe either explicitly
    // (the UniversePlace junction table) or by being that universe's start place (Universe.StartPlaceId).
    // Returns std::nullopt if neither link exists in this database.
    std::optional<int64_t> GetUniverseIdForPlace(int64_t placeId);

    // Returns a universe's start (root) place id, or std::nullopt if the universe isn't in this
    // database or has no start place set.
    std::optional<int64_t> GetStartPlaceIdForUniverse(int64_t universeId);

    // Returns the Name column of an item, or std::nullopt when the row is absent or its name is NULL.
    std::optional<std::string> GetItemName(ItemType type, int64_t id);

    // Returns the owning user's id for an item that has a UserId column (Asset, Universe, ...), or
    // std::nullopt when the row is absent or unowned by a user.
    std::optional<int64_t> GetCreatorUserId(ItemType type, int64_t id);

    // The columns the toolbox/marketplace endpoints need to describe a single asset.
    struct AssetSummary {
        int64_t Id {0};
        std::string Name;
        std::string Description;
        int Type {0};                 // Roblox::AssetType value (also the API "typeId")
        std::optional<int64_t> UserId;
        std::optional<int64_t> GroupId;
        int64_t Created {0};          // unix epoch seconds (0 if unset)
        int64_t Updated {0};          // unix epoch seconds (0 if unset)
    };

    // Returns the ids of assets matching a toolbox-style query: an optional asset type
    // (Roblox::AssetType::None matches any) and an optional name keyword (substring match), newest
    // first. limit <= 0 falls back to a sane default.
    std::vector<int64_t> SearchAssetIds(Roblox::AssetType type, const std::string &keyword, int limit, int offset);

    // Reads the descriptive columns of a single asset, or std::nullopt when it isn't in this database.
    std::optional<AssetSummary> GetAssetSummary(int64_t id);

    /* Bundle functions */
    SqlDb::Response AddAssetToBundle(int64_t bundleId, int64_t assetId);
    SqlDb::Response RemoveAssetFromBundle(int64_t bundleId, int64_t assetId);

    SqlDb::Response AddAssetToOutfit(int64_t outfitId, int64_t assetId);
    SqlDb::Response RemoveAssetFromOutfit(int64_t outfitId, int64_t assetId);

    /* User functions */
    SqlDb::Response AddAssetToUserCharacter(int64_t userId, int64_t assetId);
    SqlDb::Response RemoveAssetFromUserCharacter(int64_t userId, int64_t assetId);

    // Stores a user's avatar headshot image (fetched from Roblox) as User.HeadshotThumbnailHash so the
    // SDK can preview the user. Upserts the blob.
    SqlDb::Response AttachHeadshotToUser(int64_t userId, const std::vector<unsigned char> &data);

    // Stores a user's full-body avatar render in User.BustThumbnailHash (the body-image slot). Upserts.
    SqlDb::Response AttachBodyShotToUser(int64_t userId, const std::vector<unsigned char> &data);

    std::vector<unsigned char> RetrieveImageData(NoobWarrior::ItemType itemType, int64_t id);

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
private:
    bool VerifyIntegrityOfMigration();
    bool MigrateToLatestVersion();

    // Shared helpers for the asset auxiliary/junction tables. Each builds and runs a single
    // prepared statement; column names come from code (the same trusted-input assumption as
    // AddItem/UpdateItem), never from end users.
    // Inserts a raw, generic row into an arbitrary table (used by ImportItem). Columns the live
    // table doesn't have are skipped, mirroring AddItem's schema-drift tolerance. Returns false on
    // a prepare/step failure.
    bool InsertRawRow(const std::string &table, const SqlRow &row, const std::set<std::string> &existingColumns);

    SqlDb::Response UpsertAuxAssetRow(const std::string &table, int64_t id, const SqlRow &row);
    SqlDb::Response DetachAuxAssetRow(const std::string &table, int64_t id, const SqlRow &row);
    SqlDb::Response AddAssetLink(const std::string &table, int64_t ownerId, int64_t assetId);
    SqlDb::Response RemoveAssetLink(const std::string &table, int64_t ownerId, int64_t assetId);

    // Drops a content-addressed blob only when no table still references its hash.
    void GarbageCollectBlobIfOrphaned(const std::string &hash);

    // Names of every user table in the database (excludes SQLite's internal sqlite_* tables).
    std::vector<std::string> GetTableNames();

    // Column names that actually exist in `table` (read from the live schema). Lets AddItem/UpdateItem
    // tolerate schema drift: a column a newer build expects but an older database file never got is
    // skipped instead of failing the whole statement.
    std::set<std::string> GetColumnNames(const std::string &table);

    // Loads a BlobStorage blob by hash and returns it ready to display: un-zstd'd (storage compression)
    // and un-gzip'd (some Roblox asset bodies). Empty vector if the hash is blank/absent/undecodable.
    std::vector<unsigned char> DecodeImageBlob(const std::string &hash);

    // Columns of `table` whose declared foreign key points at BlobStorage(Hash). Discovered from
    // the live schema via PRAGMA foreign_key_list, so it stays correct as migrations add tables.
    std::vector<std::string> GetBlobHashColumns(const std::string &table);

    // Inserts into `out` the (non-null, non-empty) blob hashes held by the BlobStorage-referencing
    // columns of every row in `table` where `whereColumn` = id. Used to remember which blobs a
    // cascading delete might orphan, before the rows holding those hashes are removed.
    void CollectRowBlobHashes(const std::string &table, const std::string &whereColumn, int64_t id,
                              std::set<std::string> &out);

    std::filesystem::path mPath;
    bool mAutoCommit;
    bool mDirty;

    std::string mMigrationFailMsg;
};
}
