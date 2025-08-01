/**
 * Notes
 *
 * There can be multiple records with the same Id, but with different version numbers.
 * It is used for storing multiple versions of an item.
 *
 * For the "UserId" and "GroupId" fields, either one of these two fields have to be null, and the other must not be null.
 * There cannot be a scenario where both the "UserId" and "GroupId" fields are filled in.
 *
 * Also, just because an asset is owned by a "UserId" or "GroupId" does not necessarily mean it is in their
 * corresponding inventories.
 *
 * The following fields aim to represent historical data and are not meant to be updated in a live database:
 * "Sales", "Favorites", "Likes", "Dislikes"
 * If such an item is mutable, these values will be added on top of through conventional means, like storing them
 * in a separate table (ex: "UserLikesDislikes", or "Transaction")
 */
static const char *schema_asset = R"***(
CREATE TABLE IF NOT EXISTS "Asset" (
    "Id"	INTEGER NOT NULL,
	"Version"	INTEGER DEFAULT 1,
	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	"Name"	TEXT,
	"Description"	TEXT,
	"Created"	INTEGER,
	"Updated"	INTEGER,
	"Type"	INTEGER,
	"Icon"	INTEGER,
	"Thumbnails"	TEXT,
	"UserId"	INTEGER,
	"GroupId"	INTEGER,
	"PriceInRobux"	INTEGER,
	"PriceInTickets"	INTEGER,
	"ContentRatingTypeId"	INTEGER,
	"MinimumMembershipLevel"	INTEGER,
	"IsPublicDomain"	INTEGER,
	"IsForSale"	INTEGER,
	"IsNew"	INTEGER,
	"LimitedType"	INTEGER,
	"Remaining"	INTEGER,
	"Sales"	INTEGER,
    "Favorites"	INTEGER,
    "Likes"	INTEGER,
	"Dislikes"	INTEGER,
	"Data"	BLOB,
	PRIMARY KEY("Id","Version"),
	FOREIGN KEY("GroupId") REFERENCES "Group"("Id"),
	FOREIGN KEY("UserId") REFERENCES "User"("Id"),
    CONSTRAINT "ONLY_ONE_VALUE" CHECK((UserId IS NULL OR GroupId IS NULL) AND NOT (UserId IS NULL AND GroupId IS NULL))
);
)***";