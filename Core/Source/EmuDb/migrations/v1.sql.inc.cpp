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
static const char *migration_v1 = R"***(
CREATE TABLE IF NOT EXISTS "Meta" (
    "Key"	TEXT NOT NULL UNIQUE,
	"Value"	TEXT,
	PRIMARY KEY("Key")
);

CREATE TABLE IF NOT EXISTS "BlobStorage" (
    "Hash"	TEXT NOT NULL,
	"Blob"	BLOB NOT NULL,
	PRIMARY KEY("Hash")
);

CREATE TABLE IF NOT EXISTS "LoginSession" (
    "Token"	TEXT NOT NULL UNIQUE,
    "CreationTimestamp"	INTEGER DEFAULT (unixepoch()),
    "LastUsedTimestamp"	INTEGER DEFAULT (unixepoch()),
	"UserId"	INTEGER NOT NULL,
    "Ip"     TEXT,
    "Location"     TEXT,
    "Device"     TEXT,
	PRIMARY KEY("Token"),
    FOREIGN KEY("UserId") REFERENCES "User"("Id")
);

INSERT INTO Meta (Key, Value) VALUES
    ("Title", "Untitled"),
    ("Description", "No description available."),
    ("Version", "1.0.0"),
    ("Author", "N/A"),
    ("Icon", ""),
    ("Mutable", "1"),
    ("CompressionType", "0"),
    ("OnlyEnableIfServerWithPlaceFromThisDatabaseIsRunning", "0"),
    ("TakeHigherPriorityIfServerWithPlaceFromThisDatabaseIsRunning", "0");
)***";