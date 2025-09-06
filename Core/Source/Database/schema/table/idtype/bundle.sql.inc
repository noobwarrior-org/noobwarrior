static const char *schema_bundle = R"***(
CREATE TABLE IF NOT EXISTS "Bundle" (
    "Id"	INTEGER NOT NULL,
	"Version"	INTEGER DEFAULT 1,
	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	"Name"	TEXT,
	"Description"	TEXT,
	"Created"	INTEGER,
	"Updated"	INTEGER,
	"Type"	INTEGER NOT NULL,
	"UserId"	INTEGER,
	"GroupId"	INTEGER,
	"PriceInRobux"	INTEGER,
	"ContentRatingTypeId"	INTEGER,
	"MinimumMembershipLevel"	INTEGER,
	"IsForSale"	INTEGER,
	"IsLimited"	INTEGER,
	"IsLimitedUnique"	INTEGER,
	"IsNew"	INTEGER,
	"Remaining"	INTEGER,
	"Historical_Sales"	INTEGER,
    "Historical_Favorites"	INTEGER,
	PRIMARY KEY("Id","Version"),
    FOREIGN KEY("GroupId") REFERENCES "Group"("Id"),
	FOREIGN KEY("UserId") REFERENCES "User"("Id"),
    CONSTRAINT "ONLY_ONE_VALUE" CHECK((UserId IS NULL OR GroupId IS NULL) AND NOT (UserId IS NULL AND GroupId IS NULL))
);
)***";