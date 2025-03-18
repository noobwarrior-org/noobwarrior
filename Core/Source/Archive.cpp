// === noobWarrior ===
// File: Archive.cpp
// Started by: Hattozo
// Started on: 2/17/2025
// Description: Encapsulates a SQLite database and creates tables containing Roblox assets and other kinds of data
#include <NoobWarrior/Archive.h>
#include <NoobWarrior/NoobWarrior.h>

#include <cstdio>
#include <cstring>

static std::string MetaKv[5][2] = {
    {"Title", "Archive"},
    {"Description", "No description available."},
    {"Author", "N/A"},
    {"Version", "v1.0"}, // Not to be confused with the version of the database format, this is meant for the author to set.
    // The table can only take strings, so we're encoding this in base64.
    // The base64 in this decodes to a default .png file.
    {"Icon", "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"}
};
static std::string TableNames[17] = {
    // Meta
    "Meta",
    // Id Types
    "Asset", "Badge", "Bundle", "DevProduct", "Group", "Pass", "Universe", "User",
    // User-Related Tables
    "UserFriends", "UserFollowers", "UserFollowing", "UserInventory", "UserFavorites",
    // Group-Related Tables
    "GroupAuditLog", "GroupAllies", "GroupEnemies"
};
static std::string TableSchema[12] = {
    // Meta (0)
    R"(
    "Name"	TEXT NOT NULL UNIQUE,
	"Value"	TEXT,
	PRIMARY KEY("Name")
    )",
    /**
     * Notes
     *
     * The "Recorded" field specifies the date of when an item was archived by the creator of an archive.
     * Alongside the id itself, it uniquely identifies a record in the database and is used for storing multiple versions of an item.
     * There can be multiple records with the same Id, but with different "Recorded" dates.
     *
     * For the "UserId" and "GroupId" fields, either one of these two fields have to be null, and the other must not be null.
     * There cannot be a scenario where both the "UserId" and "GroupId" fields are filled in.
     */
    // Asset (1)
    R"(
    "Id"	INTEGER NOT NULL,
	"Recorded"	INTEGER DEFAULT (unixepoch()),
    "Version"	INTEGER,
	"Name"	TEXT,
	"Description"	TEXT,
	"Created"	INTEGER,
	"Updated"	INTEGER,
	"Type"	INTEGER,
	"UserId"	INTEGER,
	"GroupId"	INTEGER,
	"PriceInRobux"	INTEGER,
	"ContentRatingTypeId"	INTEGER,
	"MinimumMembershipLevel"	INTEGER,
	"IsPublicDomain"	INTEGER,
	"IsForSale"	INTEGER,
	"IsLimited"	INTEGER,
	"IsLimitedUnique"	INTEGER,
	"IsNew"	INTEGER,
	"Remaining"	INTEGER,
	"Sales"	INTEGER,
    "Favorites"	INTEGER,
    "Likes"	INTEGER,
	"Dislikes"	INTEGER,
	"Data"	BLOB,
	PRIMARY KEY("Id","Recorded","Version"),
	FOREIGN KEY("GroupId") REFERENCES "Group"("Id"),
	FOREIGN KEY("UserId") REFERENCES "User"("Id"),
    CONSTRAINT "ONLY_ONE_VALUE" CHECK((UserId IS NULL OR GroupId IS NULL) AND NOT (UserId IS NULL AND GroupId IS NULL))
    )",
    // Badge (2)
    R"(
    "Id"	INTEGER NOT NULL,
	"Recorded"	INTEGER DEFAULT (unixepoch()),
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
	PRIMARY KEY("Id","Recorded"),
    FOREIGN KEY("GroupId") REFERENCES "Group"("Id"),
    FOREIGN KEY("UniverseId") REFERENCES "Universe"("Id"),
	FOREIGN KEY("UserId") REFERENCES "User"("Id"),
    CONSTRAINT "ONLY_ONE_VALUE" CHECK((UserId IS NULL OR GroupId IS NULL) AND NOT (UserId IS NULL AND GroupId IS NULL))
    )",
    // Bundle (3)
    R"(
    "Id"	INTEGER NOT NULL,
	"Recorded"	INTEGER DEFAULT (unixepoch()),
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
	PRIMARY KEY("Id","Recorded"),
    FOREIGN KEY("GroupId") REFERENCES "Group"("Id"),
	FOREIGN KEY("UserId") REFERENCES "User"("Id"),
    CONSTRAINT "ONLY_ONE_VALUE" CHECK((UserId IS NULL OR GroupId IS NULL) AND NOT (UserId IS NULL AND GroupId IS NULL))
    )",
    // DevProduct (4)
    R"(
    "Id"	INTEGER NOT NULL,
	"Recorded"	INTEGER DEFAULT (unixepoch()),
	"Name"	TEXT,
	"Description"	TEXT,
	"Created"	INTEGER,
	"Updated"	INTEGER,
	"PriceInRobux"	INTEGER,
	"Image"	INTEGER,
	"UserId"	INTEGER,
	"GroupId"	INTEGER,
	"UniverseId"	INTEGER NOT NULL,
	PRIMARY KEY("Id","Recorded"),
    FOREIGN KEY("GroupId") REFERENCES "Group"("Id"),
    FOREIGN KEY("UniverseId") REFERENCES "Universe"("Id"),
	FOREIGN KEY("UserId") REFERENCES "User"("Id"),
    CONSTRAINT "ONLY_ONE_VALUE" CHECK((UserId IS NULL OR GroupId IS NULL) AND NOT (UserId IS NULL AND GroupId IS NULL))
    )",
    // Group (5)
    R"(
    "Id"	INTEGER NOT NULL,
	"Recorded"	INTEGER DEFAULT (unixepoch()),
	"Name"	TEXT,
	"Description"	TEXT,
    "Created"	INTEGER,
	"OwnerId"	INTEGER,
	"EmblemId"	INTEGER,
    "MemberCount"	INTEGER,
	"Roles"	TEXT,
    "SocialLinks"	TEXT,
    "Funds"	INTEGER,
    "Shout"	TEXT,
    "ShoutUserId"	INTEGER,
    "ShoutTimestamp"	INTEGER,
	PRIMARY KEY("Id","Recorded"),
	FOREIGN KEY("OwnerId") REFERENCES "User"("Id")
    )",
    // Pass (6)
    R"(
    "Id"	INTEGER NOT NULL,
	"Recorded"	INTEGER DEFAULT (unixepoch()),
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
	PRIMARY KEY("Id","Recorded"),
	FOREIGN KEY("GroupId") REFERENCES "Group"("Id"),
	FOREIGN KEY("UniverseId") REFERENCES "Universe"("Id"),
	FOREIGN KEY("UserId") REFERENCES "User"("Id"),
    CONSTRAINT "ONLY_ONE_VALUE" CHECK((UserId IS NULL OR GroupId IS NULL) AND NOT (UserId IS NULL AND GroupId IS NULL))
    )",
    // Universe (7)
    R"(
    "Id"	INTEGER NOT NULL,
	"Recorded"	INTEGER DEFAULT (unixepoch()),
	"Name"	TEXT,
	"Description"	INTEGER,
	"Created"	INTEGER,
	"Updated"	INTEGER,
	"UserId"	INTEGER,
	"GroupId"	INTEGER,
	"Icon"	INTEGER,
	"Thumbnails"	TEXT,
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
	PRIMARY KEY("Id","Recorded"),
	FOREIGN KEY("GroupId") REFERENCES "User"("Id"),
	FOREIGN KEY("UserId") REFERENCES "User"("Id"),
    CONSTRAINT "ONLY_ONE_VALUE" CHECK((UserId IS NULL OR GroupId IS NULL) AND NOT (UserId IS NULL AND GroupId IS NULL))
    )",
    // User (8)
    R"(
    "Id"	INTEGER NOT NULL,
	"Recorded"	INTEGER DEFAULT (unixepoch()),
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
	PRIMARY KEY("Id","Recorded")
    )",
    // UserFriends (9)
    R"(
    "UserId"	INTEGER NOT NULL,
	"FriendId"	INTEGER NOT NULL,
    "Recorded"	INTEGER DEFAULT (unixepoch()),
	"FriendsSince"	INTEGER,
	PRIMARY KEY("UserId","FriendId","Recorded"),
	FOREIGN KEY("FriendId") REFERENCES "User"("Id"),
	FOREIGN KEY("UserId") REFERENCES "User"("Id")
    )",
    // UserFollowers (10)
R"(
    "UserId"	INTEGER NOT NULL,
    "FollowerId"	INTEGER NOT NULL,
    "Recorded"	INTEGER DEFAULT (unixepoch()),
    "FollowedSince"	INTEGER,
    PRIMARY KEY("UserId","FollowerId","Recorded"),
    FOREIGN KEY("FollowerId") REFERENCES "User"("Id"),
    FOREIGN KEY("UserId") REFERENCES "User"("Id")
    )",
    // UserFollowing (11)
R"(
    "UserId"	INTEGER NOT NULL,
    "FollowingId"	INTEGER NOT NULL,
    "Recorded"	INTEGER DEFAULT (unixepoch()),
    "FollowingSince"	INTEGER,
    PRIMARY KEY("UserId","FollowingId","Recorded"),
    FOREIGN KEY("FollowingId") REFERENCES "User"("Id"),
    FOREIGN KEY("UserId") REFERENCES "User"("Id")
    )"
};

using namespace NoobWarrior;

Archive::Archive() :
    mDatabase(nullptr),
    mInitialized(false)
{};

int Archive::Open(const std::string &path) {
    mPath = path;
    int val = sqlite3_open_v2(mPath.c_str(), &mDatabase, SQLITE_OPEN_READWRITE, nullptr);
    if (val != SQLITE_OK)
        return val;
    // disable auto-commit mode by explicitly initiating a transaction
    val = sqlite3_exec(mDatabase, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
    if (val != SQLITE_OK) {
        sqlite3_close_v2(mDatabase);
        return val;
    }

    switch (GetDatabaseVersion()) {
    case -1: // -1 indicates that something has went terribly wrong. how did that happen?
        sqlite3_close_v2(mDatabase);
        return -5;
    case 0: // 0 indicates to us that we have a brand new SQLite database, as there is no noobWarrior version that starts at 0.
        // no migrating required, just set the database version and go
        if (SetDatabaseVersion(NOOBWARRIOR_ARCHIVE_VERSION) != SQLITE_OK) { sqlite3_close_v2(mDatabase); return -6; }
        break;
    }

    // create all tables that do not exist
    for (int i = 0; i < NOOBWARRIOR_ARRAY_SIZE(TableNames); i++) {
        char *statement = (char*)malloc(strlen("CREATE TABLE IF NOT EXISTS '' ();") + strlen(TableNames[i].c_str()) + strlen(TableSchema[i].c_str()) + 1);
        sprintf(statement, "CREATE TABLE IF NOT EXISTS '%s' (%s);", TableNames[i].c_str(), TableSchema[i].c_str());
        int execVal = sqlite3_exec(mDatabase, statement, nullptr, nullptr, nullptr);
        free(statement);
        if (execVal != SQLITE_OK) {
            sqlite3_close_v2(mDatabase);
            return execVal;
        }
        // sqlite3_stmt *stmt;
        // int stmtVal = sqlite3_prepare_v2(mDatabase, statement, -1, &stmt, nullptr);
        // if (stmtVal != SQLITE_OK) {
        //     sqlite3_close(mDatabase);
        //     return stmtVal;
        // }
        // int stepVal = sqlite3_step(stmt);
        // if (stepVal != SQLITE_DONE) {
        //     sqlite3_close(mDatabase);
        //     return stepVal;
        // }
        // if (sqlite3_step(stmt) != SQLITE_ROW) {
        //     // this table does not exist yet.
        //     char statement[1000];
        //     sprintf(statement, "CREATE TABLE %s (%s);", IdTypes[i].c_str(), IdSchema[i].c_str());
        // }
    }

    // and initialize some keys
    for (int i = 0; i < NOOBWARRIOR_ARRAY_SIZE(MetaKv); i++) {
        char *statement = (char*)malloc(strlen("INSERT OR IGNORE INTO Meta('Name', 'Value') VALUES('', '')") + strlen(MetaKv[i][0].c_str()) + strlen(MetaKv[i][1].c_str()) + 1);
        sprintf(statement, "INSERT OR IGNORE INTO Meta('Name', 'Value') VALUES('%s', '%s')", MetaKv[i][0].c_str(), MetaKv[i][1].c_str());
        int execVal = sqlite3_exec(mDatabase, statement, nullptr, nullptr, nullptr);
        free(statement);
        if (execVal != SQLITE_OK) {
            sqlite3_close_v2(mDatabase);
            return execVal;
        }
    }
    mInitialized = true;
    return SQLITE_OK;
}

int Archive::Close() {
    return mDatabase != nullptr ? sqlite3_close_v2(mDatabase) : 0;
}

int Archive::SaveAs(const std::string &path) {
    if (!mInitialized) return -1;
    sqlite3 *newDb;
    sqlite3_backup *backup;

    FILE *file = fopen(path.c_str(), "w");
    if (file == nullptr)
        return -2;
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
    return val;
}

int Archive::GetDatabaseVersion() {
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

int Archive::SetDatabaseVersion(int version) {
    int val = -1;
    sqlite3_stmt *stmt;
    val = sqlite3_prepare_v2(mDatabase, "PRAGMA user_version=?", 23, &stmt, nullptr); // 23 because 22 bytes of text + null terminator.
    if (val != SQLITE_OK)
        goto cleanup;
    sqlite3_bind_int(stmt, 1, version);
    val = sqlite3_step(stmt);
cleanup:
    sqlite3_finalize(stmt);
    return val;
}

int Archive::WriteChangesToDisk() {
    int val = -1;
    val = sqlite3_exec(mDatabase, "COMMIT;", nullptr, nullptr, nullptr);
    if (val != SQLITE_OK)
        goto fuck;
    val = sqlite3_exec(mDatabase, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr); // and disable auto-commit mode again
fuck:
    return val;
}

bool Archive::IsDirty() {
    // Reasoning for this is because sqlite3_get_autocommit() returns zero if there is an on-going transaction in the database.
    // If there is an on-going transaction, shit is going on behind the scenes and you should be minding your business.
    return mInitialized ? !sqlite3_get_autocommit(mDatabase) : false;
}

std::string Archive::GetSqliteErrorMsg() {
    return sqlite3_errmsg(mDatabase);
}

std::string Archive::GetTitle() {
    return "Untitled";
}

int Archive::AddAsset(Roblox::AssetDetails *asset) {
    if (!mInitialized) return -1;

    // int execVal = sqlite3_exec(mDatabase, statement, nullptr, nullptr, nullptr);
}

std::vector<unsigned char> Archive::RetrieveFile(int64_t id, IdType type) {
    const char *tbl = IdTypeAsString(type);
    if (*tbl == '\0' || !mInitialized) return {};
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(mDatabase, "SELECT * FROM Asset WHERE Id = ?;", -1, &stmt, NULL);
}