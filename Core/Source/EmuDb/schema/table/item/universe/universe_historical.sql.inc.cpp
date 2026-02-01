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
 * This represents historical data and is not meant to be updated in a live database.
 * All real-time data is interpolated with the data in this table.
 */
static const char *schema_universe_historical = R"***(
CREATE TABLE IF NOT EXISTS "UniverseHistorical" (
    "Id"	INTEGER NOT NULL,
	"Snapshot"	INTEGER NOT NULL,
    "Favorites"	INTEGER,
    "Likes"	INTEGER,
	"Dislikes"	INTEGER,
    PRIMARY KEY("Id","Snapshot"),
	FOREIGN KEY("Id", "Snapshot") REFERENCES "Universe"("Id", "Snapshot")
);
)***";