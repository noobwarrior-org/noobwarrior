static const char *schema_user_previous_names = R"***(
CREATE TABLE IF NOT EXISTS "UserPreviousNames" (
    "Name"	TEXT NOT NULL,
    "UserId"	INTEGER NOT NULL,
    "ChangedTimestamp"	INTEGER DEFAULT (unixepoch()),
	PRIMARY KEY("Name"),
	FOREIGN KEY("UserId") REFERENCES "User"("Id")
);
)***";