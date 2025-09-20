static const char *schema_set_asset = R"***(
CREATE TABLE IF NOT EXISTS "SetAsset" (
    "Id"	INTEGER NOT NULL,
	"Version"	INTEGER NOT NULL,
	"AssetId"	INTEGER NOT NULL,
	"AssetVersion"	INTEGER NOT NULL,
	PRIMARY KEY("Id","Version","AssetId","AssetVersion"),
	FOREIGN KEY("Id", "Version") REFERENCES "Set"("Id", "Version"),
	FOREIGN KEY("AssetId", "AssetVersion") REFERENCES "Asset"("Id", "Version")
);
)***";