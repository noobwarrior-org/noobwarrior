/**
 * The following fields aim to represent historical data and are not meant to be updated in a live database:
 * "MemberCount"
 * If such an item is mutable, these values will be added on top of through conventional means, like storing them
 * in a separate table (ex: "GroupMember")
 */
static const char *schema_group = R"***(
CREATE TABLE IF NOT EXISTS "Group" (
    "Id"	INTEGER NOT NULL,
	"Snapshot"	INTEGER NOT NULL,
	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	"Name"	TEXT,
	"Description"	TEXT,
    "Created"	INTEGER,
	"OwnerId"	INTEGER,
	"ImageId"	INTEGER,
	"ImageSnapshot"	INTEGER,
    "Funds"	INTEGER NOT NULL DEFAULT 0,
    "Shout"	TEXT,
    "ShoutUserId"	INTEGER,
    "ShoutTimestamp"	INTEGER,
	"EnemyDeclarationsEnabled"	INTEGER NOT NULL DEFAULT 0,
	PRIMARY KEY("Id","Snapshot"),
	FOREIGN KEY("OwnerId") REFERENCES "User"("Id"),
	FOREIGN KEY("ShoutUserId") REFERENCES "User"("Id"),
	FOREIGN KEY("ImageId", "ImageSnapshot") REFERENCES "Asset"("Id", "Snapshot")
);
)***";