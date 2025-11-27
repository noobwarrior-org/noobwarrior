static const char *schema_group_social_link = R"***(
CREATE TABLE IF NOT EXISTS "GroupSocialLink" (
    "Id"	INTEGER NOT NULL,
	"Snapshot"	INTEGER NOT NULL,
    "LinkType"	INTEGER NOT NULL,
    "Url"	TEXT NOT NULL,
    "Title"	TEXT NOT NULL,
    PRIMARY KEY("Id","Snapshot"),
	FOREIGN KEY("Id", "Snapshot") REFERENCES "Group"("Id", "Snapshot")
);
)***";