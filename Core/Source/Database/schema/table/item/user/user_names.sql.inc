static const char *schema_user_names = R"***(
CREATE TABLE IF NOT EXISTS "UserNames" (
    "Name"	TEXT COLLATE NOCASE NOT NULL,
    "Id"	INTEGER NOT NULL,
    "ChangedTimestamp"	INTEGER DEFAULT (unixepoch()),
	PRIMARY KEY("Name"),
	FOREIGN KEY("Id") REFERENCES "User"("Id")
);
)***";