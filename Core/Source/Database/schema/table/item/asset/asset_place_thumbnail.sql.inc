static const char *schema_asset_place_thumbnail = R"***(
CREATE TABLE IF NOT EXISTS "AssetPlaceThumbnail" (
    "Id"	INTEGER NOT NULL,
	"Snapshot"	INTEGER NOT NULL,
	"Thumbnail"	INTEGER NOT NULL,
	PRIMARY KEY("Id","Snapshot","Thumbnail"),
	FOREIGN KEY("Id", "Snapshot") REFERENCES "Asset"("Id", "Snapshot"),
	FOREIGN KEY("Thumbnail") REFERENCES "Asset"("Id")
);
)***";