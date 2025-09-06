static const char *schema_group_role = R"***(
CREATE TABLE IF NOT EXISTS "GroupRole" (
    "GroupId"	INTEGER NOT NULL,
	"GroupVersion"	INTEGER NOT NULL,
	"Rank"	INTEGER NOT NULL,
	"Created"	INTEGER,
	"Updated"	INTEGER,
    "FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	PRIMARY KEY("GroupId","GroupVersion","Rank"),
	FOREIGN KEY("GroupId", "GroupVersion") REFERENCES "Group"("Id", "Version")
);
)***";