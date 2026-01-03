/**
 * The following fields aim to represent historical data and are not meant to be updated in a live database:
 * "FriendCount", "FollowersCount", "FollowingCount"
 * If such an item is mutable, these values will be added on top of through conventional means, like storing them
 * in a separate table (ex: "UserFriends", "UserFollowers", "UserFollowing")
 */
static const char *schema_user = R"***(
CREATE TABLE IF NOT EXISTS "User" (
    "Id"	INTEGER NOT NULL,
	"Snapshot"	INTEGER NOT NULL,
	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	"Name"	TEXT,
	"DisplayName"	TEXT,
	"Groups"	TEXT,
	"RobloxBadges"	TEXT,
	"Outfits"	TEXT,
	"CharacterAppearance"	TEXT,
    "Bio"	TEXT,
    "Status"	TEXT,
	"JoinDate"	INTEGER,
	"LastOnline"	INTEGER,
	"PlaceVisits"	INTEGER,
    "Historical_FriendCount"	INTEGER,
    "Historical_FollowersCount"	INTEGER,
	"Historical_FollowingCount"	INTEGER,
	"Rank"	INTEGER,
	"HeadshotThumbnailHash"	TEXT,
	"BustThumbnailHash"	TEXT,
	PRIMARY KEY("Id","Snapshot"),
	FOREIGN KEY("Name") REFERENCES "UserNames"("Name"),
	FOREIGN KEY("HeadshotThumbnailHash") REFERENCES "BlobStorage"("Hash"),
	FOREIGN KEY("BustThumbnailHash") REFERENCES "BlobStorage"("Hash")
);
)***";