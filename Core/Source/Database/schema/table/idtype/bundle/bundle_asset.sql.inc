static const char *schema_bundle_asset = R"***(
CREATE TABLE IF NOT EXISTS "BundleAsset" (
    "Id"	INTEGER NOT NULL,
	"Version"	INTEGER NOT NULL,
	"AssetId"	INTEGER NOT NULL,
	"AssetVersion"	INTEGER NOT NULL,
	PRIMARY KEY("Id","Version","AssetId","AssetVersion"),
	FOREIGN KEY("Id", "Version") REFERENCES "Bundle"("Id", "Version"),
	FOREIGN KEY("AssetId", "AssetVersion") REFERENCES "Asset"("Id", "Version")
);
)***";