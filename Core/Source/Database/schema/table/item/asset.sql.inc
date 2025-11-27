/**
 * Notes
 *
 * There can be multiple records with the same Id, but with different snapshots.
 * It is used for storing backups of an item.
 *
 * For the "UserId" and "GroupId" fields, either one of these two fields have to be null, and the other must not be null.
 * There cannot be a scenario where both the "UserId" and "GroupId" fields are filled in.
 *
 * Also, just because an asset is owned by a "UserId" or "GroupId" does not necessarily mean it is in their
 * corresponding inventories.
 *
 * "Public" makes the asset uncopylocked if it is a Place. For other kinds of assets, it just makes it off-sale and undownloadable.
 */
static const char *schema_asset = R"***(
CREATE TABLE IF NOT EXISTS "Asset" (
    "Id"	INTEGER NOT NULL,
	"Snapshot"	INTEGER NOT NULL,
	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	"Name"	TEXT NOT NULL,
	"Description"	TEXT,
	"Created"	INTEGER,
	"Updated"	INTEGER,
	"Type"	INTEGER NOT NULL,
	"ImageId"	INTEGER,
	"ImageSnapshot"	INTEGER,
	"UserId"	INTEGER,
	"GroupId"	INTEGER,
	"Public"	INTEGER,
	PRIMARY KEY("Id","Snapshot"),
	FOREIGN KEY("ImageId", "ImageSnapshot") REFERENCES "Asset"("Id", "Snapshot"),
	FOREIGN KEY("UserId") REFERENCES "User"("Id"),
	FOREIGN KEY("GroupId") REFERENCES "Group"("Id"),
	CONSTRAINT "TYPE_CANNOT_BE_NULL" CHECK(Type IS NOT NULL AND Type > 0),
    CONSTRAINT "ONLY_ONE_CREATOR_VALUE" CHECK((UserId IS NULL OR GroupId IS NULL) AND NOT (UserId IS NULL AND GroupId IS NULL))
);
)***";