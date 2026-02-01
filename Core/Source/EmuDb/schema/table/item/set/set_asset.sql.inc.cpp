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
static const char *schema_set_asset = R"***(
CREATE TABLE IF NOT EXISTS "SetAsset" (
    "Id"	INTEGER NOT NULL,
	"Snapshot"	INTEGER NOT NULL,
	"AssetId"	INTEGER NOT NULL,
	"AssetSnapshot"	INTEGER NOT NULL,
	PRIMARY KEY("Id","Snapshot","AssetId","AssetSnapshot"),
	FOREIGN KEY("Id", "Snapshot") REFERENCES "Set"("Id", "Snapshot"),
	FOREIGN KEY("AssetId", "AssetSnapshot") REFERENCES "Asset"("Id", "Snapshot")
);
)***";