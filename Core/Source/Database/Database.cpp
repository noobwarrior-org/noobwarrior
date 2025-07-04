// === noobWarrior ===
// File: Database.cpp
// Started by: Hattozo
// Started on: 2/17/2025
// Description: Encapsulates a SQLite database and creates tables containing Roblox assets and other kinds of data
#include <NoobWarrior/Database/Database.h>
#include <NoobWarrior/NoobWarrior.h>

#include <nlohmann/json.hpp>

#include <cstdio>
#include <cstring>

static const char* MetaKv[][2] = {
	//////////////// Metadata ////////////////
    {"Title", "Database"},
    {"Description", "No description available."},
    {"Author", "N/A"},
    {"Version", "v1.0"}, // Not to be confused with the version of the database format, this is meant for the author to set.
    // The table can only take strings, so we're encoding this in base64.
    // The base64 in this decodes to a default .png file.
    {"Icon", "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"},
	//////////////// Booleans ////////////////
	/** Should parts of the database be able to be modified by guests during runtime?
	 * (Ex: Adding records to friend table by friending someone in-game, liking/disliking a game, uploading UGC, etc.)
	 */
	{"Mutable", "false"}
};
static const char* TableNames[] = {
    // Meta
    "Meta",
    // Id Types
    "Asset", "Badge", "Bundle", "DevProduct", "Group", "Pass", "Universe", "User",
    // User-Related Tables
    "UserFriends", "UserFollowers", "UserFollowing", "UserInventory", "UserFavorites", "UserLikesDislikes",
    // Group-Related Tables
    "GroupRole", "GroupMember", "GroupWall", "GroupLog", "GroupAlly", "GroupEnemy",
	// Misc
	"Transaction"
};
static const char* TableSchema[] = {
    // Meta (0)
    R"(
    "Name"	TEXT NOT NULL UNIQUE,
	"Value"	TEXT,
	PRIMARY KEY("Name")
    )",
    /**
     * Notes
     *
     * There can be multiple records with the same Id, but with different version numbers.
     * It is used for storing multiple versions of an item.
     *
     * For the "UserId" and "GroupId" fields, either one of these two fields have to be null, and the other must not be null.
     * There cannot be a scenario where both the "UserId" and "GroupId" fields are filled in.
     *
	 * Also, just because an asset is owned by a "UserId" or "GroupId" does not necessarily mean it is in their
	 * corresponding inventories.
     *
     * The following fields aim to represent historical data and are not meant to be updated in a live database:
     * "Sales", "Favorites", "Likes", "Dislikes"
     * If such an item is mutable, these values will be added on top of through conventional means, like storing them
     * in a separate table (ex: "UserLikesDislikes", or "Transaction")
     */
    // Asset (1)
    R"(
    "Id"	INTEGER NOT NULL,
	"Version"	INTEGER NOT NULL,
	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	"Name"	TEXT,
	"Description"	TEXT,
	"Created"	INTEGER,
	"Updated"	INTEGER,
	"Type"	INTEGER,
	"Icon"	INTEGER,
	"Thumbnails"	TEXT,
	"UserId"	INTEGER,
	"GroupId"	INTEGER,
	"PriceInRobux"	INTEGER,
	"PriceInTickets"	INTEGER,
	"ContentRatingTypeId"	INTEGER,
	"MinimumMembershipLevel"	INTEGER,
	"IsPublicDomain"	INTEGER,
	"IsForSale"	INTEGER,
	"IsNew"	INTEGER,
	"LimitedType"	INTEGER,
	"Remaining"	INTEGER,
	"Sales"	INTEGER,
    "Favorites"	INTEGER,
    "Likes"	INTEGER,
	"Dislikes"	INTEGER,
	"Data"	BLOB,
	PRIMARY KEY("Id","Version"),
	FOREIGN KEY("GroupId") REFERENCES "Group"("Id"),
	FOREIGN KEY("UserId") REFERENCES "User"("Id"),
    CONSTRAINT "ONLY_ONE_VALUE" CHECK((UserId IS NULL OR GroupId IS NULL) AND NOT (UserId IS NULL AND GroupId IS NULL))
    )",
    // Badge (2)
    R"(
    "Id"	INTEGER NOT NULL,
	"Version"	INTEGER NOT NULL,
	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	"Name"	TEXT,
	"Description"	TEXT,
	"Created"	INTEGER,
	"Updated"	INTEGER,
	"Image"	INTEGER,
	"Awarded"	INTEGER,
    "AwardedYesterday"	INTEGER,
	"UserId"	INTEGER,
	"GroupId"	INTEGER,
	"UniverseId"	INTEGER NOT NULL,
	PRIMARY KEY("Id","Version"),
    FOREIGN KEY("GroupId") REFERENCES "Group"("Id"),
    FOREIGN KEY("UniverseId") REFERENCES "Universe"("Id"),
	FOREIGN KEY("UserId") REFERENCES "User"("Id"),
    CONSTRAINT "ONLY_ONE_VALUE" CHECK((UserId IS NULL OR GroupId IS NULL) AND NOT (UserId IS NULL AND GroupId IS NULL))
    )",
    // Bundle (3)
    R"(
    "Id"	INTEGER NOT NULL,
	"Version"	INTEGER NOT NULL,
	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	"Name"	TEXT,
	"Description"	TEXT,
	"Created"	INTEGER,
	"Updated"	INTEGER,
	"Type"	INTEGER NOT NULL,
	"UserId"	INTEGER,
	"GroupId"	INTEGER,
	"PriceInRobux"	INTEGER,
	"ContentRatingTypeId"	INTEGER,
	"MinimumMembershipLevel"	INTEGER,
	"IsForSale"	INTEGER,
	"IsLimited"	INTEGER,
	"IsLimitedUnique"	INTEGER,
	"IsNew"	INTEGER,
	"Remaining"	INTEGER,
	"Sales"	INTEGER,
    "Favorites"	INTEGER,
	"Items"	TEXT,
	PRIMARY KEY("Id","Version"),
    FOREIGN KEY("GroupId") REFERENCES "Group"("Id"),
	FOREIGN KEY("UserId") REFERENCES "User"("Id"),
    CONSTRAINT "ONLY_ONE_VALUE" CHECK((UserId IS NULL OR GroupId IS NULL) AND NOT (UserId IS NULL AND GroupId IS NULL))
    )",
    // DevProduct (4)
    R"(
    "Id"	INTEGER NOT NULL,
	"Version"	INTEGER NOT NULL,
	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	"Name"	TEXT,
	"Description"	TEXT,
	"Created"	INTEGER,
	"Updated"	INTEGER,
	"PriceInRobux"	INTEGER,
	"Image"	INTEGER,
	"UserId"	INTEGER,
	"GroupId"	INTEGER,
	"UniverseId"	INTEGER NOT NULL,
	PRIMARY KEY("Id","Version"),
    FOREIGN KEY("GroupId") REFERENCES "Group"("Id"),
    FOREIGN KEY("UniverseId") REFERENCES "Universe"("Id"),
	FOREIGN KEY("UserId") REFERENCES "User"("Id"),
    CONSTRAINT "ONLY_ONE_VALUE" CHECK((UserId IS NULL OR GroupId IS NULL) AND NOT (UserId IS NULL AND GroupId IS NULL))
    )",
	/**
	 * The following fields aim to represent historical data and are not meant to be updated in a live database:
	 * "MemberCount"
	 * If such an item is mutable, these values will be added on top of through conventional means, like storing them
	 * in a separate table (ex: "GroupMember")
	 */
    // Group (5)
    R"(
    "Id"	INTEGER NOT NULL,
	"Version"	INTEGER NOT NULL,
	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	"Name"	TEXT,
	"Description"	TEXT,
    "Created"	INTEGER,
	"OwnerId"	INTEGER,
	"EmblemId"	INTEGER,
    "MemberCount"	INTEGER,
    "SocialLinks"	TEXT,
    "Funds"	INTEGER,
    "Shout"	TEXT,
    "ShoutUserId"	INTEGER,
    "ShoutTimestamp"	INTEGER,
	"EnemyDeclarationsEnabled"	INTEGER DEFAULT 0,
	PRIMARY KEY("Id","Version"),
	FOREIGN KEY("OwnerId") REFERENCES "User"("Id"),
	FOREIGN KEY("ShoutUserId") REFERENCES "User"("Id"),
	FOREIGN KEY("EmblemId") REFERENCES "Asset"("Id")
    )",
    // Pass (6)
    R"(
    "Id"	INTEGER NOT NULL,
	"Version"	INTEGER NOT NULL,
	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	"Name"	TEXT,
	"Description"	TEXT,
	"Created"	INTEGER,
	"Updated"	INTEGER,
	"Image"	INTEGER,
	"UserId"	INTEGER,
	"GroupId"	INTEGER,
	"UniverseId"	INTEGER NOT NULL,
	"PriceInRobux"	INTEGER NOT NULL,
	"IsForSale"	INTEGER,
    "Likes"	INTEGER,
	"Dislikes"	INTEGER,
	PRIMARY KEY("Id","Version"),
	FOREIGN KEY("GroupId") REFERENCES "Group"("Id"),
	FOREIGN KEY("UniverseId") REFERENCES "Universe"("Id"),
	FOREIGN KEY("UserId") REFERENCES "User"("Id"),
    CONSTRAINT "ONLY_ONE_VALUE" CHECK((UserId IS NULL OR GroupId IS NULL) AND NOT (UserId IS NULL AND GroupId IS NULL))
    )",
    // Universe (7)
    R"(
    "Id"	INTEGER NOT NULL,
	"Version"	INTEGER NOT NULL,
	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	"Name"	TEXT,
	"Description"	INTEGER,
	"Created"	INTEGER,
	"Updated"	INTEGER,
	"StartPlaceId"	INTEGER,
	"UserId"	INTEGER,
	"GroupId"	INTEGER,
	"Active"	INTEGER,
	"AccessType"	INTEGER,
	"PaymentType"	INTEGER,
	"Genre"	INTEGER,
	"Subgenre"	INTEGER,
	"AgeRating"	INTEGER,
	"SocialLinks"	TEXT,
	"SupportedDevices"	TEXT,
	"Favorites"	INTEGER,
	"Likes"	INTEGER,
	"Dislikes"	INTEGER,
	"Visits"	INTEGER,
	PRIMARY KEY("Id","Version"),
	FOREIGN KEY("GroupId") REFERENCES "User"("Id"),
	FOREIGN KEY("UserId") REFERENCES "User"("Id"),
    CONSTRAINT "ONLY_ONE_VALUE" CHECK((UserId IS NULL OR GroupId IS NULL) AND NOT (UserId IS NULL AND GroupId IS NULL))
    )",
	/**
	 * The following fields aim to represent historical data and are not meant to be updated in a live database:
	 * "FriendCount", "FollowersCount", "FollowingCount"
	 * If such an item is mutable, these values will be added on top of through conventional means, like storing them
	 * in a separate table (ex: "UserFriends", "UserFollowers", "UserFollowing")
	 */
    // User (8)
    R"(
    "Id"	INTEGER NOT NULL,
	"Version"	INTEGER NOT NULL,
	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	"Name"	TEXT,
	"PreviousNames"	TEXT,
	"Groups"	TEXT,
	"RobloxBadges"	TEXT,
	"Outfits"	TEXT,
	"CharacterAppearance"	TEXT,
    "Bio"	TEXT,
    "Status"	TEXT,
	"JoinDate"	INTEGER,
	"LastOnline"	INTEGER,
	"PlaceVisits"	INTEGER,
    "FriendCount"	INTEGER,
    "FollowersCount"	INTEGER,
	"FollowingCount"	INTEGER,
	PRIMARY KEY("Id","Version")
    )",
    // UserFriends (9)
    R"(
    "UserId"	INTEGER NOT NULL,
	"FriendId"	INTEGER NOT NULL,
	"FriendedSince"	INTEGER,
	"UnfriendedSince"	INTEGER,
    "FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	PRIMARY KEY("UserId","FriendId","FriendedSince"),
	FOREIGN KEY("FriendId") REFERENCES "User"("Id"),
	FOREIGN KEY("UserId") REFERENCES "User"("Id")
    )",
    // UserFollowers (10)
R"(
    "UserId"	INTEGER NOT NULL,
    "FollowerId"	INTEGER NOT NULL,
	"FollowedSince"	INTEGER,
	"UnfollowedSince"	INTEGER,
    "FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
    PRIMARY KEY("UserId","FollowerId","FollowedSince"),
    FOREIGN KEY("FollowerId") REFERENCES "User"("Id"),
    FOREIGN KEY("UserId") REFERENCES "User"("Id")
    )",
    // UserFollowing (11)
R"(
    "UserId"	INTEGER NOT NULL,
    "FollowingId"	INTEGER NOT NULL,
	"FollowingSince"	INTEGER,
	"UnfollowingSince"	INTEGER,
    "FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
    PRIMARY KEY("UserId","FollowingId","FollowingSince"),
    FOREIGN KEY("FollowingId") REFERENCES "User"("Id"),
    FOREIGN KEY("UserId") REFERENCES "User"("Id")
    )",
	/**
	 * Fill in only one of these "AssetId", "BadgeId", "BundleId", "GroupId", "PassId", "UniverseId" for each record
	 * if you don't want the universe to explode. Doing polymorphic association in SQL is very stupid
	 *
	 * The same also goes for UserFavorites and UserLikesDislikes
	 */
	// UserInventory (12)
R"(
    "UserId"	INTEGER NOT NULL,
	"UserVersion"	INTEGER NOT NULL,

	"AssetId"	INTEGER,
	"BadgeId"	INTEGER,
	"BundleId"	INTEGER,
	"GroupId"	INTEGER,
	"PassId"	INTEGER,
	"UniverseId"	INTEGER,

	"PurchasedTimestamp"	INTEGER,
    "FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	PRIMARY KEY("UserId","UserVersion","AssetId","BadgeId","BundleId","GroupId","PassId","UniverseId"),
    FOREIGN KEY("UserId") REFERENCES "User"("Id"),
	FOREIGN KEY("UserVersion") REFERENCES "User"("Version"),

	FOREIGN KEY("AssetId") REFERENCES "Asset"("Id"),
	FOREIGN KEY("BadgeId") REFERENCES "Badge"("Id"),
	FOREIGN KEY("BundleId") REFERENCES "Bundle"("Id"),
	FOREIGN KEY("GroupId") REFERENCES "Group"("Id"),
	FOREIGN KEY("PassId") REFERENCES "Pass"("Id"),
	FOREIGN KEY("UniverseId") REFERENCES "Universe"("Id")
    )",
	// UserFavorites (13)
R"(
    "UserId"	INTEGER NOT NULL,
	"UserVersion"	INTEGER NOT NULL,

	"AssetId"	INTEGER,
	"BadgeId"	INTEGER,
	"BundleId"	INTEGER,
	"GroupId"	INTEGER,
	"PassId"	INTEGER,
	"UniverseId"	INTEGER,

	"FavoritedTimestamp"	INTEGER,
    "FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	PRIMARY KEY("UserId","UserVersion","AssetId","BadgeId","BundleId","GroupId","PassId","UniverseId"),
    FOREIGN KEY("UserId") REFERENCES "User"("Id"),
	FOREIGN KEY("UserVersion") REFERENCES "User"("Version"),

	FOREIGN KEY("AssetId") REFERENCES "Asset"("Id"),
	FOREIGN KEY("BadgeId") REFERENCES "Badge"("Id"),
	FOREIGN KEY("BundleId") REFERENCES "Bundle"("Id"),
	FOREIGN KEY("GroupId") REFERENCES "Group"("Id"),
	FOREIGN KEY("PassId") REFERENCES "Pass"("Id"),
	FOREIGN KEY("UniverseId") REFERENCES "Universe"("Id")
    )",
	// UserLikesDislikes (14)
	R"(
	"UserId"	INTEGER NOT NULL,

	"LikedSince"	INTEGER,
	"UnlikedSince"	INTEGER,

	"Type"	INTEGER NOT NULL,

	"AssetId"	INTEGER,
	"BadgeId"	INTEGER,
	"BundleId"	INTEGER,
	"GroupId"	INTEGER,
	"PassId"	INTEGER,
	"UniverseId"	INTEGER,

	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),

	PRIMARY KEY("UserId","LikedSince","AssetId","BadgeId","BundleId","GroupId","PassId","UniverseId"),
    FOREIGN KEY("UserId") REFERENCES "User"("Id"),

	FOREIGN KEY("AssetId") REFERENCES "Asset"("Id"),
	FOREIGN KEY("BadgeId") REFERENCES "Badge"("Id"),
	FOREIGN KEY("BundleId") REFERENCES "Bundle"("Id"),
	FOREIGN KEY("GroupId") REFERENCES "Group"("Id"),
	FOREIGN KEY("PassId") REFERENCES "Pass"("Id"),
	FOREIGN KEY("UniverseId") REFERENCES "Universe"("Id")
	)",
	// GroupRole (15)
	R"(
	"GroupId"	INTEGER NOT NULL,
	"GroupVersion"	INTEGER NOT NULL,
	"Rank"	INTEGER NOT NULL,

	"Created"	INTEGER,
	"Updated"	INTEGER,
    "FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	PRIMARY KEY("GroupId","GroupVersion","Rank"),
	FOREIGN KEY("GroupId") REFERENCES "Group"("Id"),
	FOREIGN KEY("GroupVersion") REFERENCES "Group"("Version")
    )",
	// GroupMember (16)
	R"(
	"GroupId"	INTEGER NOT NULL,
	"UserId"	INTEGER NOT NULL,
	"JoinedTimestamp"	INTEGER,
	"LeftTimestamp"	INTEGER,
	"Rank"	INTEGER NOT NULL,
    "FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	PRIMARY KEY("GroupId","UserId"),
	FOREIGN KEY("GroupId") REFERENCES "Group"("Id"),
    FOREIGN KEY("UserId") REFERENCES "User"("Id")
    )",
	// GroupWall (17)
	R"(
	"GroupId"	INTEGER NOT NULL,
	"PostId"	INTEGER,
    "UserId"	INTEGER,
	"Message"	TEXT,
	"Date"	INTEGER,
    "FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	PRIMARY KEY("GroupId","PostId"),
    FOREIGN KEY("UserId") REFERENCES "User"("Id"),
	FOREIGN KEY("GroupId") REFERENCES "Group"("Id")
    )",
	// GroupLog (18)
	R"(
	"GroupId"	INTEGER NOT NULL,
	"LogId"	INTEGER,
    "UserId"	INTEGER,
	"LogType"	INTEGER,
	"UserData"	TEXT,
	"Date"	INTEGER,
    "FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	PRIMARY KEY("GroupId","LogId"),
    FOREIGN KEY("UserId") REFERENCES "User"("Id"),
	FOREIGN KEY("GroupId") REFERENCES "Group"("Id")
    )",
	// GroupAlly (19)
	R"(
	"GroupId"	INTEGER NOT NULL,
	"AllyId"	INTEGER NOT NULL,
	"AlliedSince"	INTEGER,
	"UnalliedSince"	INTEGER,
	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	PRIMARY KEY("GroupId","AllyId","AlliedSince"),
    FOREIGN KEY("GroupId") REFERENCES "Group"("Id"),
	FOREIGN KEY("AllyId") REFERENCES "Group"("Id")
	)",
	// GroupEnemy (20)
	R"(
	"GroupId"	INTEGER NOT NULL,
	"EnemyId"	INTEGER NOT NULL,
	"AntagonizedSince"	INTEGER,
	"UnantagonizedSince"	INTEGER,
	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	PRIMARY KEY("GroupId","EnemyId","AntagonizedSince"),
    FOREIGN KEY("GroupId") REFERENCES "Group"("Id"),
	FOREIGN KEY("EnemyId") REFERENCES "Group"("Id")
	)",
	// Transaction (21)
	R"(
	"TransactionId"	INTEGER,
	"Date"	INTEGER,
	"Amount"	INTEGER,

	"BuyerUserId"	INTEGER,
	"BuyerGroupId"	INTEGER,

	"SellerUserId"	INTEGER,
	"SellerGroupId"	INTEGER,

	"AssetId"	INTEGER,
	"BadgeId"	INTEGER,
	"BundleId"	INTEGER,
	"DevProductId"	INTEGER,
	"GroupId"	INTEGER,
	"PassId"	INTEGER,
	"UniverseId"	INTEGER,

	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	PRIMARY KEY("TransactionId"),

	FOREIGN KEY("BuyerUserId") REFERENCES "User"("Id"),
	FOREIGN KEY("BuyerGroupId") REFERENCES "Group"("Id"),

	FOREIGN KEY("SellerUserId") REFERENCES "User"("Id"),
	FOREIGN KEY("SellerGroupId") REFERENCES "Group"("Id"),

	FOREIGN KEY("AssetId") REFERENCES "Asset"("Id"),
	FOREIGN KEY("BadgeId") REFERENCES "Badge"("Id"),
	FOREIGN KEY("BundleId") REFERENCES "Bundle"("Id"),
	FOREIGN KEY("DevProductId") REFERENCES "DevProduct"("Id"),
	FOREIGN KEY("GroupId") REFERENCES "Group"("Id"),
	FOREIGN KEY("PassId") REFERENCES "Pass"("Id"),
	FOREIGN KEY("UniverseId") REFERENCES "Universe"("Id"),

    CONSTRAINT "ONLY_ONE_VALUE1" CHECK((BuyerUserId IS NULL OR BuyerGroupId IS NULL) AND NOT (BuyerUserId IS NULL AND BuyerGroupId IS NULL)),
	CONSTRAINT "ONLY_ONE_VALUE2" CHECK((SellerUserId IS NULL OR SellerGroupId IS NULL) AND NOT (SellerUserId IS NULL AND SellerGroupId IS NULL))
	)",
};

using namespace NoobWarrior;

Database::Database(bool autocommit) :
    mDatabase(nullptr),
    mInitialized(false),
	mAutoCommit(autocommit),
	mDirty(false)
{};

DatabaseResponse Database::Open(const std::string &path) {
    mPath = path;
    int val = sqlite3_open_v2(reinterpret_cast<const char *>(mPath.c_str()), &mDatabase, SQLITE_OPEN_READWRITE, nullptr);
    if (val != SQLITE_OK)
        return DatabaseResponse::CouldNotOpen;
	if (!mAutoCommit) {
		// disable auto-commit mode by explicitly initiating a transaction
		val = sqlite3_exec(mDatabase, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
		if (val != SQLITE_OK) {
			sqlite3_close_v2(mDatabase);
			return DatabaseResponse::Failed;
		}
	}

    switch (GetDatabaseVersion()) {
    case -1: // -1 indicates that something has went terribly wrong. how did that happen?
        sqlite3_close_v2(mDatabase);
        return DatabaseResponse::CouldNotGetVersion;
    case 0: // 0 indicates to us that we have a brand new SQLite database, as there is no noobWarrior version that starts at 0.
        // no migrating required, just set the database version and go
        if (SetDatabaseVersion(NOOBWARRIOR_DATABASE_VERSION) != SQLITE_OK) { sqlite3_close_v2(mDatabase); return DatabaseResponse::CouldNotSetVersion; }
        break;
    }

    // create all tables that do not exist
    for (int i = 0; i < NOOBWARRIOR_ARRAY_SIZE(TableNames); i++) {
    	std::string stmtStr = std::format("CREATE TABLE IF NOT EXISTS '{}' ({});", TableNames[i], TableSchema[i]);
        int execVal = sqlite3_exec(mDatabase, stmtStr.c_str(), nullptr, nullptr, nullptr);
        if (execVal != SQLITE_OK) {
            sqlite3_close_v2(mDatabase);
            return DatabaseResponse::CouldNotCreateTable;
        }
    }

    // and initialize some keys
    for (int i = 0; i < NOOBWARRIOR_ARRAY_SIZE(MetaKv); i++) {
        std::string stmtStr = "INSERT INTO Meta('Name', 'Value') VALUES(?, ?)";
    	sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(mDatabase, stmtStr.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        	Out("Database", "Failed to prepare");
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
    return sqlite3_exec(mDatabase, stmtStr.c_str(), nullptr, nullptr, nullptr);
}

DatabaseResponse Database::SaveAs(const std::string &path) {
	if (!mInitialized) return DatabaseResponse::NotInitialized;
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
		sqlite3_backup_step(backup, -1);
		sqlite3_backup_finish(backup);
	}
	cleanup:
		sqlite3_close_v2(newDb);
	return DatabaseResponse::Success;
}

DatabaseResponse Database::WriteChangesToDisk() {
	if (mAutoCommit) return DatabaseResponse::DidNothing; // You don't have to save

    if (sqlite3_exec(mDatabase, "COMMIT;", nullptr, nullptr, nullptr) != SQLITE_OK)
    	return DatabaseResponse::Failed;
	if (sqlite3_exec(mDatabase, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr) != SQLITE_OK) // and disable auto-commit mode again
		return DatabaseResponse::Failed;
    return DatabaseResponse::Success;
}

bool Database::IsDirty() {
	if (mAutoCommit) return false;
    return mDirty;
}

void Database::MarkDirty() {
	if (mAutoCommit) return; // every single thing is saved, it's never dirty
	mDirty = true;
}

std::string Database::GetSqliteErrorMsg() {
    return sqlite3_errmsg(mDatabase);
}

std::string Database::GetTitle() {
    return "Untitled";
}

std::string Database::GetFileName() {
    return mPath.filename().string();
}

std::filesystem::path Database::GetFilePath() {
    if (mPath.compare(":memory:"))
        return "";
    return mPath;
}

DatabaseResponse Database::AddAsset(Asset *asset) {
    if (!mInitialized) return DatabaseResponse::NotInitialized;

	MarkDirty();

    // int execVal = sqlite3_exec(mDatabase, statement, nullptr, nullptr, nullptr);
}

std::vector<unsigned char> Database::RetrieveContentData(int64_t id, IdType type) {
    const char *tbl = IdTypeAsString(type);
    if (*tbl == '\0' || !mInitialized) return {};
    sqlite3_stmt *stmt;
	sqlite3_prepare_v2(mDatabase, "SELECT * FROM ? WHERE Id = ? ORDER BY Version DESC LIMIT 1;", -1, &stmt, nullptr);
	sqlite3_bind_text(stmt, 1, tbl, -1, nullptr);
	sqlite3_bind_int64(stmt, 2, id);

	if (sqlite3_step(stmt) == SQLITE_DONE) {
		for (int i = 0; i < sqlite3_column_count(stmt); i++) {
			if (strncmp(sqlite3_column_name(stmt, i), "Data", 4) == 0) {
				std::vector<unsigned char> data;
				unsigned char *buf = (unsigned char*)sqlite3_column_blob(stmt, i);
				data.assign(buf, buf + sqlite3_column_bytes(stmt, i));
				return data;
			}
		}
	}

	return {};
}

Asset Database::GetAsset(int64_t id) {
	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(mDatabase, "SELECT * FROM Asset WHERE Id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, id);

	while (sqlite3_step(stmt) == SQLITE_ROW) {

	}
}

std::vector<Asset> Database::SearchAssets(const SearchOptions &opt) {
	if (!mInitialized) return {};

	std::vector<Asset> assets;

	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(mDatabase, "SELECT * FROM Asset LIMIT ? OFFSET ?;", -1, &stmt, nullptr);
	sqlite3_bind_int(stmt, 1, opt.Limit);
	sqlite3_bind_int(stmt, 2, opt.Offset);

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		Asset asset;
		asset.Id = sqlite3_column_int(stmt, 0);
		asset.Version = sqlite3_column_int(stmt, 1);
		asset.FirstRecorded = sqlite3_column_int(stmt, 2);
		asset.LastRecorded = sqlite3_column_int(stmt, 3);
		asset.Name = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
		asset.Description = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
		asset.Created = sqlite3_column_int(stmt, 6);
		asset.Updated = sqlite3_column_int(stmt, 7);
		asset.Type = static_cast<Roblox::AssetType>(sqlite3_column_int(stmt, 8));
		asset.Icon = sqlite3_column_int(stmt, 9);
		try { asset.Thumbnails = nlohmann::json::parse(sqlite3_column_text(stmt, 10)); } catch (nlohmann::detail::parse_error &e) { asset.Thumbnails = "[]"; }
		{
			// The database table has separate fields for the creator ID, "UserId" and "GroupId", so that referencing foreign keys can be possible.
			asset.CreatorType = sqlite3_column_type(stmt, 11) != SQLITE_NULL
									? Roblox::CreatorType::User
									: Roblox::CreatorType::Group;
			asset.CreatorId = sqlite3_column_type(stmt, 11) != SQLITE_NULL
								  ? sqlite3_column_int(stmt, 11)
								  : sqlite3_column_int(stmt, 12);
		}
		asset.PriceInRobux = sqlite3_column_int(stmt, 13);
		asset.PriceInTickets = sqlite3_column_int(stmt, 14);
		asset.ContentRatingTypeId = sqlite3_column_int(stmt, 15);
		asset.MinimumMembershipLevel = sqlite3_column_int(stmt, 16);
		asset.IsPublicDomain = sqlite3_column_int(stmt, 17);
		asset.IsForSale = sqlite3_column_int(stmt, 18);
		asset.IsNew = sqlite3_column_int(stmt, 19);
		asset.LimitedType = static_cast<Roblox::LimitedType>(sqlite3_column_int(stmt, 20));
		asset.Remaining = sqlite3_column_int(stmt, 21);

		// Historical data
		asset.Sales = sqlite3_column_int(stmt, 22);
		asset.Favorites = sqlite3_column_int(stmt, 23);
		asset.Likes = sqlite3_column_int(stmt, 24);
		asset.Dislikes = sqlite3_column_int(stmt, 25);

		assets.push_back(asset);
	}

	sqlite3_finalize(stmt);
	return assets;
}
