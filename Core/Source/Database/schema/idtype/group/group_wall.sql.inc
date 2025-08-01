static const char *schema_group_wall = R"***(
CREATE TABLE IF NOT EXISTS "GroupWall" (
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
);
)***";