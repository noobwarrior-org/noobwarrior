static const char *schema_bundle_asset = R"***(
CREATE TABLE IF NOT EXISTS "BundleAsset" (
    "Id"	INTEGER NOT NULL,
	"Snapshot"	INTEGER NOT NULL,
	"AssetId"	INTEGER NOT NULL,
	"AssetSnapshot"	INTEGER NOT NULL,
	PRIMARY KEY("Id","Snapshot","AssetId","AssetSnapshot"),
	FOREIGN KEY("Id", "Snapshot") REFERENCES "Bundle"("Id", "Snapshot"),
	FOREIGN KEY("AssetId", "AssetSnapshot") REFERENCES "Asset"("Id", "Snapshot")
);
)***";