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
static const char *schema_group_wall = R"***(
CREATE TABLE IF NOT EXISTS "GroupWall" (
    "Id"	INTEGER NOT NULL,
	"PostId"	INTEGER,
    "UserId"	INTEGER,
	"Message"	TEXT,
	"Date"	INTEGER,
    "FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	PRIMARY KEY("Id","PostId"),
    FOREIGN KEY("UserId") REFERENCES "User"("Id"),
	FOREIGN KEY("Id") REFERENCES "Group"("Id")
);
)***";