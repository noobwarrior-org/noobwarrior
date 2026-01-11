static const char *schema_asset_place_gear_type = R"***(
CREATE TABLE IF NOT EXISTS "AssetPlaceGearType" (
    "Id"	INTEGER NOT NULL,
	"Snapshot"	INTEGER NOT NULL,
    "GearType"	INTEGER UNIQUE,
    PRIMARY KEY("Id","Snapshot"),
	FOREIGN KEY("Id", "Snapshot") REFERENCES "Asset"("Id", "Snapshot")
);
)***";