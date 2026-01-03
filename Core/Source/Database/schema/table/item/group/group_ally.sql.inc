static const char *schema_group_ally = R"***(
CREATE TABLE IF NOT EXISTS "GroupAlly" (
    "Id"	INTEGER NOT NULL,
	"AllyId"	INTEGER NOT NULL,
	"AlliedSince"	INTEGER,
	"UnalliedSince"	INTEGER,
	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	PRIMARY KEY("Id","AllyId","AlliedSince"),
    FOREIGN KEY("Id") REFERENCES "Group"("Id"),
	FOREIGN KEY("AllyId") REFERENCES "Group"("Id")
);
)***";