static const char *schema_set_asset = R"***(
CREATE TABLE IF NOT EXISTS "SetAsset" (
    "Id"	INTEGER NOT NULL,
	"Snapshot"	INTEGER NOT NULL,
	"AssetId"	INTEGER NOT NULL,
	"AssetSnapshot"	INTEGER NOT NULL,
	PRIMARY KEY("Id","Snapshot","AssetId","AssetSnapshot"),
	FOREIGN KEY("Id", "Snapshot") REFERENCES "Set"("Id", "Snapshot"),
	FOREIGN KEY("AssetId", "AssetSnapshot") REFERENCES "Asset"("Id", "Snapshot")
);
)***";