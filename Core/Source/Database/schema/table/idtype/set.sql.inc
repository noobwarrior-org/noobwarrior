static const char *schema_set = R"***(
CREATE TABLE IF NOT EXISTS "Set" (
    "Id"	INTEGER NOT NULL,
	"Version"	INTEGER DEFAULT 1,
	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	"Name"	TEXT,
	"Description"	TEXT,
	"Created"	INTEGER,
	"Updated"	INTEGER,
    "ImageId"	INTEGER,
	"ImageVersion"	INTEGER,
	"UserId"	INTEGER,
	"GroupId"	INTEGER,
	"Historical_Subscribers"	INTEGER,
	PRIMARY KEY("Id","Version"),
    FOREIGN KEY("ImageId", "ImageVersion") REFERENCES "Asset"("Id", "Version"),
    FOREIGN KEY("UserId") REFERENCES "User"("Id"),
    FOREIGN KEY("GroupId") REFERENCES "Group"("Id"),
    CONSTRAINT "ONLY_ONE_VALUE" CHECK((UserId IS NULL OR GroupId IS NULL) AND NOT (UserId IS NULL AND GroupId IS NULL))
);
)***";