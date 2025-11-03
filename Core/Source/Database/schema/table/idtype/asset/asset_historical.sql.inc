/**
 * This represents historical data and is not meant to be updated in a live database.
 * All real-time data is interpolated with the data in this table.
 */
static const char *schema_asset_historical = R"***(
CREATE TABLE IF NOT EXISTS "AssetHistorical" (
    "Id"	INTEGER NOT NULL,
	"Version"	INTEGER NOT NULL,
    "IsNew"	INTEGER,
	"Sales"	INTEGER,
    "Favorites"	INTEGER,
    "Likes"	INTEGER,
	"Dislikes"	INTEGER,
    PRIMARY KEY("Id","Version"),
	FOREIGN KEY("Id", "Version") REFERENCES "Asset"("Id", "Version")
);
)***";