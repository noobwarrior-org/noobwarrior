static const char *schema_group_log = R"***(
CREATE TABLE IF NOT EXISTS "GroupLog" (
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
);
)***";