static const char *schema_universe_misc = R"***(
CREATE TABLE IF NOT EXISTS "UniverseMisc" (
    "Id"	INTEGER NOT NULL,
	"Snapshot"	INTEGER NOT NULL,
	"Genre"	INTEGER,
	"Subgenre"	INTEGER,
    "AvatarType"	INTEGER,
    "AccessType"	INTEGER,
	"PaymentType"	INTEGER,
	"AllowPrivateServers"	INTEGER,
	"AllowDirectAccessToPlaces"	INTEGER,
	"AgeRating"	INTEGER,
	"SupportedDevices"	TEXT,
    PRIMARY KEY("Id","Snapshot"),
	FOREIGN KEY("Id", "Snapshot") REFERENCES "Universe"("Id", "Snapshot")
);
)***";