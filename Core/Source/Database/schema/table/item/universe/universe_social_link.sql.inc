static const char *schema_universe_social_link = R"***(
CREATE TABLE IF NOT EXISTS "UniverseSocialLink" (
    "Id"	INTEGER NOT NULL,
	"Snapshot"	INTEGER NOT NULL,
    "LinkType"	INTEGER NOT NULL,
    "Url"	TEXT NOT NULL,
    "Title"	TEXT NOT NULL,
    PRIMARY KEY("Id","Snapshot"),
	FOREIGN KEY("Id", "Snapshot") REFERENCES "Universe"("Id", "Snapshot")
);
)***";