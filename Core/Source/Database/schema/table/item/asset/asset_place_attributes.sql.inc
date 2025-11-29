static const char *schema_asset_place_attributes = R"***(
CREATE TABLE IF NOT EXISTS "AssetPlaceAttributes" (
    "Id"	INTEGER NOT NULL,
	"Snapshot"	INTEGER NOT NULL,
    "MaxPlayers"	INTEGER,
    "AllowDirectAccess"	INTEGER,
    "GearGenrePermission"	INTEGER,
    PRIMARY KEY("Id","Snapshot"),
	FOREIGN KEY("Id", "Snapshot") REFERENCES "Asset"("Id", "Snapshot")
);
)***";