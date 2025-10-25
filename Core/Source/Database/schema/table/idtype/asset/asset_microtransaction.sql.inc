/**
 * Anything that has to do with money, stock, and purchasing
 */
static const char *schema_asset_microtransaction = R"***(
CREATE TABLE IF NOT EXISTS "AssetMicrotransaction" (
    "Id"	INTEGER NOT NULL,
	"Version"	INTEGER NOT NULL,
    "CurrencyType"	INTEGER,
	"Price"	INTEGER,
    "LimitedType"	INTEGER,
	"Remaining"	INTEGER,
    PRIMARY KEY("Id","Version"),
	FOREIGN KEY("Id", "Version") REFERENCES "Asset"("Id", "Version")
);
)***";