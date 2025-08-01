static const char *schema_group_member = R"***(
CREATE TABLE IF NOT EXISTS "GroupMember" (
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
);
)***";