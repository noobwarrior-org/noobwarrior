/**
 * The following fields aim to represent historical data and are not meant to be updated in a live database:
 * "MemberCount"
 * If such an item is mutable, these values will be added on top of through conventional means, like storing them
 * in a separate table (ex: "GroupMember")
 */
static const char *schema_group = R"***(
CREATE TABLE IF NOT EXISTS "Group" (
    "Id"	INTEGER NOT NULL,
	"Version"	INTEGER DEFAULT 1,
	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	"Name"	TEXT,
	"Description"	TEXT,
    "Created"	INTEGER,
	"OwnerId"	INTEGER,
	"EmblemId"	INTEGER,
    "Historical_MemberCount"	INTEGER,
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
);
)***";