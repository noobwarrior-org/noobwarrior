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
/**
 * Anything that has to do with money, stock, and purchasing
 */
static const char *schema_asset_microtransaction = R"***(
CREATE TABLE IF NOT EXISTS "AssetMicrotransaction" (
    "Id"	INTEGER NOT NULL,
	"Snapshot"	INTEGER NOT NULL,
    "CurrencyType"	INTEGER,
	"Price"	INTEGER,
    "LimitedType"	INTEGER,
	"Remaining"	INTEGER,
    PRIMARY KEY("Id","Snapshot"),
	FOREIGN KEY("Id", "Snapshot") REFERENCES "Asset"("Id", "Snapshot")
);
)***";