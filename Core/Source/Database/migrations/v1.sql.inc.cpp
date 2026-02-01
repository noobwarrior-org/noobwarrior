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
    ("Icon", "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"),
    ("Mutable", "1"),
    ("CompressionType", "0"),
    ("OnlyEnableIfServerWithPlaceFromThisDatabaseIsRunning", "0"),
    ("TakeHigherPriorityIfServerWithPlaceFromThisDatabaseIsRunning", "0");
)***";