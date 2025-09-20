static const char *schema_devproduct = R"***(
CREATE TABLE IF NOT EXISTS "DevProduct" (
    "Id"	INTEGER NOT NULL,
	"Version"	INTEGER DEFAULT 1,
	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	"Name"	TEXT,
	"Description"	TEXT,
	"Created"	INTEGER,
	"Updated"	INTEGER,
	"CurrencyType"	INTEGER,
	"Price"	INTEGER,
	"ImageId"	INTEGER,
	"ImageVersion"	INTEGER,
	"UserId"	INTEGER,
	"GroupId"	INTEGER,
	"UniverseId"	INTEGER NOT NULL,
	PRIMARY KEY("Id","Version"),
	FOREIGN KEY("ImageId", "ImageVersion") REFERENCES "Asset"("Id", "Version"),
    FOREIGN KEY("GroupId") REFERENCES "Group"("Id"),
    FOREIGN KEY("UniverseId") REFERENCES "Universe"("Id"),
	FOREIGN KEY("UserId") REFERENCES "User"("Id"),
    CONSTRAINT "ONLY_ONE_VALUE" CHECK((UserId IS NULL OR GroupId IS NULL) AND NOT (UserId IS NULL AND GroupId IS NULL))
);
)***";