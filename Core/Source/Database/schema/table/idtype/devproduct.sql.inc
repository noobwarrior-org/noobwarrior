static const char *schema_devproduct = R"***(
CREATE TABLE IF NOT EXISTS "DevProduct" (
    "Id"	INTEGER NOT NULL,
	"Snapshot"	INTEGER NOT NULL,
	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	"Name"	TEXT,
	"Description"	TEXT,
	"Created"	INTEGER,
	"Updated"	INTEGER,
	"CurrencyType"	INTEGER NOT NULL,
	"Price"	INTEGER NOT NULL,
	"ImageId"	INTEGER,
	"ImageSnapshot"	INTEGER,
	"UniverseId"	INTEGER NOT NULL,
	PRIMARY KEY("Id","Snapshot"),
	FOREIGN KEY("ImageId", "ImageSnapshot") REFERENCES "Asset"("Id", "Snapshot"),
    FOREIGN KEY("UniverseId") REFERENCES "Universe"("Id"),
);
)***";