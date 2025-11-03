/**
 * Miscellaneous attributes of an asset that aren't really relevant or are rarely needed.
 */
static const char *schema_asset_misc = R"***(
CREATE TABLE IF NOT EXISTS "AssetMisc" (
    "Id"	INTEGER NOT NULL,
	"Version"	INTEGER NOT NULL,
    "MinimumMembershipLevel"	INTEGER,
    "ContentRatingTypeId"	INTEGER,
    PRIMARY KEY("Id","Version"),
	FOREIGN KEY("Id", "Version") REFERENCES "Asset"("Id", "Version")
);
)***";