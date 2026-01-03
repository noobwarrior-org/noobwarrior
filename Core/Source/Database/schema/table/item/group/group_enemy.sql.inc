static const char *schema_group_enemy = R"***(
CREATE TABLE IF NOT EXISTS "GroupEnemy" (
    "Id"	INTEGER NOT NULL,
	"EnemyId"	INTEGER NOT NULL,
	"AntagonizedSince"	INTEGER,
	"UnantagonizedSince"	INTEGER,
	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	PRIMARY KEY("Id","EnemyId","AntagonizedSince"),
    FOREIGN KEY("Id") REFERENCES "Group"("Id"),
	FOREIGN KEY("EnemyId") REFERENCES "Group"("Id")
);
)***";