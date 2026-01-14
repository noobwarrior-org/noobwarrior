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
static const char *schema_universe_misc = R"***(
CREATE TABLE IF NOT EXISTS "UniverseMisc" (
    "Id"	INTEGER NOT NULL,
	"Snapshot"	INTEGER NOT NULL,
	"Genre"	INTEGER,
	"Subgenre"	INTEGER,
    "AvatarType"	INTEGER,
    "AccessType"	INTEGER,
	"PaymentType"	INTEGER,
	"AllowPrivateServers"	INTEGER,
	"AllowDirectAccessToPlaces"	INTEGER,
	"AgeRating"	INTEGER,
	"SupportedDevices"	TEXT,
    PRIMARY KEY("Id","Snapshot"),
	FOREIGN KEY("Id", "Snapshot") REFERENCES "Universe"("Id", "Snapshot")
);
)***";