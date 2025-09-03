static const char *schema_set_asset = R"***(
CREATE TABLE IF NOT EXISTS "SetAsset" (
    "Id"	INTEGER NOT NULL,
	"Version"	INTEGER NOT NULL,
	"AssetId"	INTEGER NOT NULL,
	PRIMARY KEY("Id","Version","AssetId"),
	FOREIGN KEY("Id") REFERENCES "Set"("Id"),
	FOREIGN KEY("Version") REFERENCES "Set"("Version"),
	FOREIGN KEY("AssetId") REFERENCES "Asset"("Id")
);
)***";