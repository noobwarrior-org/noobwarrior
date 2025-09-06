static const char *schema_asset_place_thumbnail = R"***(
CREATE TABLE IF NOT EXISTS "AssetPlaceThumbnail" (
    "Id"	INTEGER NOT NULL,
	"Version"	INTEGER NOT NULL,
	"Thumbnail"	INTEGER NOT NULL,
	PRIMARY KEY("Id","Version","Thumbnail"),
	FOREIGN KEY("Id", "Version") REFERENCES "Asset"("Id", "Version"),
	FOREIGN KEY("Thumbnail") REFERENCES "Asset"("Id")
);
)***";