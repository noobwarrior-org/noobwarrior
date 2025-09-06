/**
 * Fill in only one of these "AssetId", "BadgeId", "BundleId", "GroupId", "PassId", "SetId", "UniverseId" for each record
 * if you don't want the universe to explode. Doing polymorphic association in SQL is very stupid
 */
static const char *schema_user_inventory = R"***(
CREATE TABLE IF NOT EXISTS "UserInventory" (
    "UserId"	INTEGER NOT NULL,
	"UserVersion"	INTEGER NOT NULL,

	"AssetId"	INTEGER,
	"BadgeId"	INTEGER,
	"BundleId"	INTEGER,
	"GroupId"	INTEGER,
	"PassId"	INTEGER,
	"SetId"	INTEGER,
	"UniverseId"	INTEGER,

	"PurchasedTimestamp"	INTEGER,
    "FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	PRIMARY KEY("UserId","UserVersion","AssetId","BadgeId","BundleId","GroupId","PassId","SetId","UniverseId"),
    FOREIGN KEY("UserId", "UserVersion") REFERENCES "User"("Id", "Version"),

	FOREIGN KEY("AssetId") REFERENCES "Asset"("Id"),
	FOREIGN KEY("BadgeId") REFERENCES "Badge"("Id"),
	FOREIGN KEY("BundleId") REFERENCES "Bundle"("Id"),
	FOREIGN KEY("GroupId") REFERENCES "Group"("Id"),
	FOREIGN KEY("PassId") REFERENCES "Pass"("Id"),
	FOREIGN KEY("SetId") REFERENCES "Set"("Id"),
	FOREIGN KEY("UniverseId") REFERENCES "Universe"("Id")
);
)***";