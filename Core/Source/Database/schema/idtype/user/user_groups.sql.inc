static const char *schema_user_groups = R"***(
CREATE TABLE IF NOT EXISTS "UserGroups" (
	"UserId"	INTEGER NOT NULL,
    "GroupId"	INTEGER NOT NULL,
	"JoinedTimestamp"	INTEGER,
	"LeftTimestamp"	INTEGER,
	"Rank"	INTEGER NOT NULL,
    "FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	PRIMARY KEY("UserId","GroupId"),
	FOREIGN KEY("UserId") REFERENCES "User"("Id"),
	FOREIGN KEY("GroupId") REFERENCES "Group"("Id")
);
)***";