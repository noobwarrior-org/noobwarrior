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
 * The following fields aim to represent historical data and are not meant to be updated in a live database:
 * "FriendCount", "FollowersCount", "FollowingCount"
 * If such an item is mutable, these values will be added on top of through conventional means, like storing them
 * in a separate table (ex: "UserFriends", "UserFollowers", "UserFollowing")
 */
static const char *schema_user = R"***(
CREATE TABLE IF NOT EXISTS "User" (
    "Id"	INTEGER NOT NULL,
	"Snapshot"	INTEGER NOT NULL,
	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	"Name"	TEXT,
	"DisplayName"	TEXT,
	"Groups"	TEXT,
	"RobloxBadges"	TEXT,
	"Outfits"	TEXT,
	"CharacterAppearance"	TEXT,
    "Bio"	TEXT,
    "Status"	TEXT,
	"JoinDate"	INTEGER,
	"LastOnline"	INTEGER,
	"PlaceVisits"	INTEGER,
    "Historical_FriendCount"	INTEGER,
    "Historical_FollowersCount"	INTEGER,
	"Historical_FollowingCount"	INTEGER,
	"Rank"	INTEGER,
	"HeadshotThumbnailHash"	TEXT,
	"BustThumbnailHash"	TEXT,
	PRIMARY KEY("Id","Snapshot"),
	FOREIGN KEY("Name") REFERENCES "UserNames"("Name"),
	FOREIGN KEY("HeadshotThumbnailHash") REFERENCES "BlobStorage"("Hash"),
	FOREIGN KEY("BustThumbnailHash") REFERENCES "BlobStorage"("Hash")
);
)***";