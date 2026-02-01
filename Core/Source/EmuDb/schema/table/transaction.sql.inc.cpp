/*
 * Copyright (C) 2026 Hattozo
 *
 * This file is part of noobWarrior.
 *
 * noobWarrior is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * noobWarrior is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with noobWarrior; if not, see
 * <https://www.gnu.org/licenses/>.
 */
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