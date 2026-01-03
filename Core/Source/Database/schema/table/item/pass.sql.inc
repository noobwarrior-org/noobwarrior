static const char *schema_pass = R"***(
CREATE TABLE IF NOT EXISTS "Pass" (
    "Id"	INTEGER NOT NULL,
	"Snapshot"	INTEGER NOT NULL,
	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	"Name"	TEXT,
	"Description"	TEXT,
	"Created"	INTEGER,
	"Updated"	INTEGER,
	"ImageId"	INTEGER,
	"ImageSnapshot"	INTEGER,
	"UserId"	INTEGER,
	"GroupId"	INTEGER,
	"UniverseId"	INTEGER NOT NULL,
	"PriceInRobux"	INTEGER NOT NULL,
	"IsForSale"	INTEGER,
    "Historical_Likes"	INTEGER,
	"Historical_Dislikes"	INTEGER,
	PRIMARY KEY("Id","Snapshot"),
	FOREIGN KEY("ImageId", "ImageSnapshot") REFERENCES "Asset"("Id", "Snapshot"),
	FOREIGN KEY("UserId") REFERENCES "User"("Id"),
	FOREIGN KEY("GroupId") REFERENCES "Group"("Id"),
	FOREIGN KEY("UniverseId") REFERENCES "Universe"("Id"),
    CONSTRAINT "ONLY_ONE_VALUE" CHECK((UserId IS NULL OR GroupId IS NULL) AND NOT (UserId IS NULL AND GroupId IS NULL))
);
)***";