static const char *schema_transaction = R"***(
CREATE TABLE IF NOT EXISTS "Transaction" (
    "TransactionId"	INTEGER,
	"Date"	INTEGER,
	"Amount"	INTEGER,

	"BuyerUserId"	INTEGER,
	"BuyerGroupId"	INTEGER,

	"SellerUserId"	INTEGER,
	"SellerGroupId"	INTEGER,

	"AssetId"	INTEGER,
	"BadgeId"	INTEGER,
	"BundleId"	INTEGER,
	"DevProductId"	INTEGER,
	"GroupId"	INTEGER,
	"PassId"	INTEGER,
	"UniverseId"	INTEGER,

	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	PRIMARY KEY("TransactionId"),

	FOREIGN KEY("BuyerUserId") REFERENCES "User"("Id"),
	FOREIGN KEY("BuyerGroupId") REFERENCES "Group"("Id"),

	FOREIGN KEY("SellerUserId") REFERENCES "User"("Id"),
	FOREIGN KEY("SellerGroupId") REFERENCES "Group"("Id"),

	FOREIGN KEY("AssetId") REFERENCES "Asset"("Id"),
	FOREIGN KEY("BadgeId") REFERENCES "Badge"("Id"),
	FOREIGN KEY("BundleId") REFERENCES "Bundle"("Id"),
	FOREIGN KEY("DevProductId") REFERENCES "DevProduct"("Id"),
	FOREIGN KEY("GroupId") REFERENCES "Group"("Id"),
	FOREIGN KEY("PassId") REFERENCES "Pass"("Id"),
	FOREIGN KEY("UniverseId") REFERENCES "Universe"("Id"),

    CONSTRAINT "ONLY_ONE_VALUE1" CHECK((BuyerUserId IS NULL OR BuyerGroupId IS NULL) AND NOT (BuyerUserId IS NULL AND BuyerGroupId IS NULL)),
	CONSTRAINT "ONLY_ONE_VALUE2" CHECK((SellerUserId IS NULL OR SellerGroupId IS NULL) AND NOT (SellerUserId IS NULL AND SellerGroupId IS NULL))
);
)***";