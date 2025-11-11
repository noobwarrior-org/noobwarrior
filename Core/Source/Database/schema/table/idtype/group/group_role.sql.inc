static const char *schema_group_role = R"***(
CREATE TABLE IF NOT EXISTS "GroupRole" (
    "Id"	INTEGER NOT NULL,
	"Snapshot"	INTEGER NOT NULL,
	"Rank"	INTEGER NOT NULL,
	"Created"	INTEGER,
	"Updated"	INTEGER,
    "FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	PRIMARY KEY("Id","Snapshot","Rank"),
	FOREIGN KEY("Id", "Snapshot") REFERENCES "Group"("Id", "Snapshot")
);
)***";