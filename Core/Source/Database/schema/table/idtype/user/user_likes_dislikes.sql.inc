/**
 * Fill in only one of these "AssetId", "BadgeId", "BundleId", "GroupId", "PassId", "UniverseId" for each record
 * if you don't want the universe to explode. Doing polymorphic association in SQL is very stupid
 */
static const char *schema_user_likes_dislikes = R"***(
CREATE TABLE IF NOT EXISTS "UserLikesDislikes" (
    "Id"	INTEGER NOT NULL,

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

	PRIMARY KEY("Id","LikedSince","AssetId","BadgeId","BundleId","GroupId","PassId","UniverseId"),
    FOREIGN KEY("Id") REFERENCES "User"("Id"),

	FOREIGN KEY("AssetId") REFERENCES "Asset"("Id"),
	FOREIGN KEY("BadgeId") REFERENCES "Badge"("Id"),
	FOREIGN KEY("BundleId") REFERENCES "Bundle"("Id"),
	FOREIGN KEY("GroupId") REFERENCES "Group"("Id"),
	FOREIGN KEY("PassId") REFERENCES "Pass"("Id"),
	FOREIGN KEY("UniverseId") REFERENCES "Universe"("Id")
);
)***";