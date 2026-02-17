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
static const char *migration_v3 = R"***(
CREATE TABLE IF NOT EXISTS "Asset" (
    "Id"	INTEGER NOT NULL,
	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	"Name"	TEXT NOT NULL,
	"Description"	TEXT,
	"Created"	INTEGER,
	"Updated"	INTEGER,
	"ImageId"	INTEGER,
	"UserId"	INTEGER,
	"GroupId"	INTEGER,
	"Type"	INTEGER NOT NULL,
	"Public"	INTEGER,
	PRIMARY KEY("Id"),
	FOREIGN KEY("ImageId") REFERENCES "Asset"("Id"),
	FOREIGN KEY("UserId") REFERENCES "User"("Id"),
	FOREIGN KEY("GroupId") REFERENCES "Group"("Id")
);

CREATE TABLE IF NOT EXISTS "Badge" (
    "Id"	INTEGER NOT NULL,
	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	"Name"	TEXT,
	"Description"	TEXT,
	"Created"	INTEGER,
	"Updated"	INTEGER,
	"ImageId"	INTEGER,
	"Enabled"	INTEGER,
	"Awarded"	INTEGER,
    "AwardedYesterday"	INTEGER,
	"UserId"	INTEGER,
	"GroupId"	INTEGER,
	"UniverseId"	INTEGER NOT NULL,
	PRIMARY KEY("Id"),
	FOREIGN KEY("ImageId") REFERENCES "Asset"("Id"),
	FOREIGN KEY("UserId") REFERENCES "User"("Id"),
    FOREIGN KEY("GroupId") REFERENCES "Group"("Id"),
    FOREIGN KEY("UniverseId") REFERENCES "Universe"("Id")
);

CREATE TABLE IF NOT EXISTS "Bundle" (
    "Id"	INTEGER NOT NULL,
	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	"Name"	TEXT,
	"Description"	TEXT,
	"Created"	INTEGER,
	"Updated"	INTEGER,
	"Type"	INTEGER NOT NULL,
	"UserId"	INTEGER,
	"GroupId"	INTEGER,
	"PriceInRobux"	INTEGER,
	"ContentRatingTypeId"	INTEGER,
	"MinimumMembershipLevel"	INTEGER,
	"IsForSale"	INTEGER,
	"IsLimited"	INTEGER,
	"IsLimitedUnique"	INTEGER,
	"IsNew"	INTEGER,
	"Remaining"	INTEGER,
	"Historical_Sales"	INTEGER,
    "Historical_Favorites"	INTEGER,
	PRIMARY KEY("Id"),
    FOREIGN KEY("GroupId") REFERENCES "Group"("Id"),
	FOREIGN KEY("UserId") REFERENCES "User"("Id")
);

CREATE TABLE IF NOT EXISTS "DevProduct" (
    "Id"	INTEGER NOT NULL,
	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	"Name"	TEXT,
	"Description"	TEXT,
	"Created"	INTEGER,
	"Updated"	INTEGER,
	"CurrencyType"	INTEGER NOT NULL,
	"Price"	INTEGER NOT NULL,
	"ImageId"	INTEGER,
	"UniverseId"	INTEGER NOT NULL,
	PRIMARY KEY("Id"),
	FOREIGN KEY("ImageId") REFERENCES "Asset"("Id"),
    FOREIGN KEY("UniverseId") REFERENCES "Universe"("Id")
);

CREATE TABLE IF NOT EXISTS "Group" (
    "Id"	INTEGER NOT NULL,
	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	"Name"	TEXT,
	"Description"	TEXT,
    "Created"	INTEGER,
	"OwnerId"	INTEGER,
	"ImageId"	INTEGER,
    "Funds"	INTEGER NOT NULL DEFAULT 0,
    "Shout"	TEXT,
    "ShoutUserId"	INTEGER,
    "ShoutTimestamp"	INTEGER,
	"EnemyDeclarationsEnabled"	INTEGER NOT NULL DEFAULT 0,
	PRIMARY KEY("Id"),
	FOREIGN KEY("OwnerId") REFERENCES "User"("Id"),
	FOREIGN KEY("ShoutUserId") REFERENCES "User"("Id"),
	FOREIGN KEY("ImageId") REFERENCES "Asset"("Id")
);

CREATE TABLE IF NOT EXISTS "Pass" (
    "Id"	INTEGER NOT NULL,
	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	"Name"	TEXT,
	"Description"	TEXT,
	"Created"	INTEGER,
	"Updated"	INTEGER,
	"ImageId"	INTEGER,
	"UserId"	INTEGER,
	"GroupId"	INTEGER,
	"UniverseId"	INTEGER NOT NULL,
	"PriceInRobux"	INTEGER NOT NULL,
	"IsForSale"	INTEGER,
    "Historical_Likes"	INTEGER,
	"Historical_Dislikes"	INTEGER,
	PRIMARY KEY("Id"),
	FOREIGN KEY("ImageId") REFERENCES "Asset"("Id"),
	FOREIGN KEY("UserId") REFERENCES "User"("Id"),
	FOREIGN KEY("GroupId") REFERENCES "Group"("Id"),
	FOREIGN KEY("UniverseId") REFERENCES "Universe"("Id")
);

CREATE TABLE IF NOT EXISTS "Set" (
    "Id"	INTEGER NOT NULL,
	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	"Name"	TEXT,
	"Description"	TEXT,
	"Created"	INTEGER,
	"Updated"	INTEGER,
    "ImageId"	INTEGER,
	"UserId"	INTEGER,
	"GroupId"	INTEGER,
	"Historical_Subscribers"	INTEGER,
	PRIMARY KEY("Id"),
    FOREIGN KEY("ImageId") REFERENCES "Asset"("Id"),
    FOREIGN KEY("UserId") REFERENCES "User"("Id"),
    FOREIGN KEY("GroupId") REFERENCES "Group"("Id")
);

CREATE TABLE IF NOT EXISTS "Universe" (
    "Id"	INTEGER NOT NULL,
	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	"Name"	TEXT,
	"Description"	TEXT,
	"Created"	INTEGER,
	"Updated"	INTEGER,
	"StartPlaceId"	INTEGER,
	"UserId"	INTEGER,
	"GroupId"	INTEGER,
	"Active"	INTEGER,
	"Visits"	INTEGER,
	PRIMARY KEY("Id"),
	FOREIGN KEY("UserId") REFERENCES "User"("Id"),
	FOREIGN KEY("GroupId") REFERENCES "Group"("Id")
);

CREATE TABLE IF NOT EXISTS "User" (
    "Id"	INTEGER NOT NULL,
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
	PRIMARY KEY("Id"),
	FOREIGN KEY("Name") REFERENCES "UserNames"("Name"),
	FOREIGN KEY("HeadshotThumbnailHash") REFERENCES "BlobStorage"("Hash"),
	FOREIGN KEY("BustThumbnailHash") REFERENCES "BlobStorage"("Hash")
);

CREATE TABLE IF NOT EXISTS "AssetData" (
    "Id"	INTEGER,
	"Version"	INTEGER,
	"DataHash"	TEXT,
	"AutogeneratedThumbnailHash"	TEXT,
	PRIMARY KEY("Id","Version"),
	FOREIGN KEY("Id") REFERENCES "Asset"("Id"),
	FOREIGN KEY("DataHash") REFERENCES "BlobStorage"("Hash"),
	FOREIGN KEY("AutogeneratedThumbnailHash") REFERENCES "BlobStorage"("Hash")
);

CREATE TABLE IF NOT EXISTS "AssetHistorical" (
    "Id"	INTEGER,
    "IsNew"	INTEGER,
	"Sales"	INTEGER,
    "Favorites"	INTEGER,
    "Likes"	INTEGER,
	"Dislikes"	INTEGER,
    PRIMARY KEY("Id"),
	FOREIGN KEY("Id") REFERENCES "Asset"("Id")
);

CREATE TABLE IF NOT EXISTS "AssetMicrotransaction" (
    "Id"	INTEGER,
    "CurrencyType"	INTEGER,
	"Price"	INTEGER,
    "LimitedType"	INTEGER,
	"Remaining"	INTEGER,
    PRIMARY KEY("Id"),
	FOREIGN KEY("Id") REFERENCES "Asset"("Id")
);

CREATE TABLE IF NOT EXISTS "AssetMisc" (
    "Id"	INTEGER,
    "MinimumMembershipLevel"	INTEGER,
    "ContentRatingTypeId"	INTEGER,
    PRIMARY KEY("Id"),
	FOREIGN KEY("Id") REFERENCES "Asset"("Id")
);

CREATE TABLE IF NOT EXISTS "AssetPlaceThumbnail" (
    "Id"	INTEGER,
	"Thumbnail"	INTEGER NOT NULL UNIQUE,
	PRIMARY KEY("Id"),
	FOREIGN KEY("Id") REFERENCES "Asset"("Id"),
	FOREIGN KEY("Thumbnail") REFERENCES "Asset"("Id")
);

CREATE TABLE IF NOT EXISTS "AssetPlaceAttributes" (
    "Id"	INTEGER,
    "MaxPlayers"	INTEGER,
    "AllowDirectAccess"	INTEGER,
    "GearGenrePermission"	INTEGER,
    PRIMARY KEY("Id"),
	FOREIGN KEY("Id") REFERENCES "Asset"("Id")
);

CREATE TABLE IF NOT EXISTS "AssetPlaceGearType" (
    "Id"	INTEGER,
    "GearType"	INTEGER UNIQUE,
    PRIMARY KEY("Id"),
	FOREIGN KEY("Id") REFERENCES "Asset"("Id")
);

CREATE TABLE IF NOT EXISTS "BundleAsset" (
    "Id"	INTEGER,
	"AssetId"	INTEGER,
	PRIMARY KEY("Id","AssetId"),
	FOREIGN KEY("Id") REFERENCES "Bundle"("Id"),
	FOREIGN KEY("AssetId") REFERENCES "Asset"("Id")
);

CREATE TABLE IF NOT EXISTS "UniverseMisc" (
    "Id"	INTEGER,
	"Genre"	INTEGER,
	"Subgenre"	INTEGER,
    "AvatarType"	INTEGER,
    "AccessType"	INTEGER,
	"PaymentType"	INTEGER,
	"AllowPrivateServers"	INTEGER,
	"AllowDirectAccessToPlaces"	INTEGER,
	"AgeRating"	INTEGER,
	"SupportedDevices"	TEXT,
    PRIMARY KEY("Id"),
	FOREIGN KEY("Id") REFERENCES "Universe"("Id")
);

CREATE TABLE IF NOT EXISTS "UniverseHistorical" (
    "Id"	INTEGER,
    "Favorites"	INTEGER,
    "Likes"	INTEGER,
	"Dislikes"	INTEGER,
    PRIMARY KEY("Id"),
	FOREIGN KEY("Id") REFERENCES "Universe"("Id")
);

CREATE TABLE IF NOT EXISTS "UniverseSocialLink" (
    "Id"	INTEGER,
    "LinkType"	INTEGER NOT NULL,
    "Url"	TEXT NOT NULL,
    "Title"	TEXT NOT NULL,
    PRIMARY KEY("Id"),
	FOREIGN KEY("Id") REFERENCES "Universe"("Id")
);

CREATE TABLE IF NOT EXISTS "UserFriends" (
    "Id"	INTEGER,
	"FriendId"	INTEGER,
	"FriendedSince"	INTEGER,
	"UnfriendedSince"	INTEGER,
    "FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	PRIMARY KEY("Id","FriendId","FriendedSince"),
	FOREIGN KEY("FriendId") REFERENCES "User"("Id"),
	FOREIGN KEY("Id") REFERENCES "User"("Id")
);

CREATE TABLE IF NOT EXISTS "UserGroups" (
	"Id"	INTEGER,
    "GroupId"	INTEGER,
	"JoinedTimestamp"	INTEGER,
	"LeftTimestamp"	INTEGER,
	"Rank"	INTEGER NOT NULL,
    "FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	PRIMARY KEY("Id","GroupId"),
	FOREIGN KEY("Id") REFERENCES "User"("Id"),
	FOREIGN KEY("GroupId") REFERENCES "Group"("Id")
);

CREATE TABLE IF NOT EXISTS "UserFollowers" (
    "Id"	INTEGER,
    "FollowerId"	INTEGER,
	"FollowedSince"	INTEGER,
	"UnfollowedSince"	INTEGER,
    "FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
    PRIMARY KEY("Id","FollowerId","FollowedSince"),
    FOREIGN KEY("FollowerId") REFERENCES "User"("Id"),
    FOREIGN KEY("Id") REFERENCES "User"("Id")
);

CREATE TABLE IF NOT EXISTS "UserFollowing" (
    "Id"	INTEGER,
    "FollowingId"	INTEGER,
	"FollowingSince"	INTEGER,
	"UnfollowingSince"	INTEGER,
    "FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
    PRIMARY KEY("Id","FollowingId","FollowingSince"),
    FOREIGN KEY("FollowingId") REFERENCES "User"("Id"),
    FOREIGN KEY("Id") REFERENCES "User"("Id")
);

CREATE TABLE IF NOT EXISTS "UserInventory" (
    "Id"	INTEGER,

	"AssetId"	INTEGER,
	"BadgeId"	INTEGER,
	"BundleId"	INTEGER,
	"GroupId"	INTEGER,
	"PassId"	INTEGER,
	"SetId"	INTEGER,
	"UniverseId"	INTEGER,

	"PurchasedTimestamp"	INTEGER,
    "FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	PRIMARY KEY("Id","AssetId","BadgeId","BundleId","GroupId","PassId","SetId","UniverseId"),
    FOREIGN KEY("Id") REFERENCES "User"("Id"),

	FOREIGN KEY("AssetId") REFERENCES "Asset"("Id"),
	FOREIGN KEY("BadgeId") REFERENCES "Badge"("Id"),
	FOREIGN KEY("BundleId") REFERENCES "Bundle"("Id"),
	FOREIGN KEY("GroupId") REFERENCES "Group"("Id"),
	FOREIGN KEY("PassId") REFERENCES "Pass"("Id"),
	FOREIGN KEY("SetId") REFERENCES "Set"("Id"),
	FOREIGN KEY("UniverseId") REFERENCES "Universe"("Id")
);

CREATE TABLE IF NOT EXISTS "UserFavorites" (
    "Id"	INTEGER,

	"AssetId"	INTEGER,
	"BadgeId"	INTEGER,
	"BundleId"	INTEGER,
	"GroupId"	INTEGER,
	"PassId"	INTEGER,
	"UniverseId"	INTEGER,

	"FavoritedTimestamp"	INTEGER,
    "FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	PRIMARY KEY("Id","AssetId","BadgeId","BundleId","GroupId","PassId","UniverseId"),
    FOREIGN KEY("Id") REFERENCES "User"("Id"),

	FOREIGN KEY("AssetId") REFERENCES "Asset"("Id"),
	FOREIGN KEY("BadgeId") REFERENCES "Badge"("Id"),
	FOREIGN KEY("BundleId") REFERENCES "Bundle"("Id"),
	FOREIGN KEY("GroupId") REFERENCES "Group"("Id"),
	FOREIGN KEY("PassId") REFERENCES "Pass"("Id"),
	FOREIGN KEY("UniverseId") REFERENCES "Universe"("Id")
);

CREATE TABLE IF NOT EXISTS "UserLikesDislikes" (
    "Id"	INTEGER,

	"LikedSince"	INTEGER,
	"UnlikedSince"	INTEGER,

	"Type"	INTEGER NOT NULL,

	"AssetId"	INTEGER,
	"BadgeId"	INTEGER,
	"BundleId"	INTEGER,
	"GroupId"	INTEGER,
	"PassId"	INTEGER,
	"UniverseId"	INTEGER,

	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),

	PRIMARY KEY("Id","LikedSince","AssetId","BadgeId","BundleId","GroupId","PassId","UniverseId"),
    FOREIGN KEY("Id") REFERENCES "User"("Id"),

	FOREIGN KEY("AssetId") REFERENCES "Asset"("Id"),
	FOREIGN KEY("BadgeId") REFERENCES "Badge"("Id"),
	FOREIGN KEY("BundleId") REFERENCES "Bundle"("Id"),
	FOREIGN KEY("GroupId") REFERENCES "Group"("Id"),
	FOREIGN KEY("PassId") REFERENCES "Pass"("Id"),
	FOREIGN KEY("UniverseId") REFERENCES "Universe"("Id")
);

CREATE TABLE IF NOT EXISTS "UserNames" (
    "Name"	TEXT COLLATE NOCASE,
    "Id"	INTEGER NOT NULL,
    "ChangedTimestamp"	INTEGER DEFAULT (unixepoch()),
	PRIMARY KEY("Name"),
	FOREIGN KEY("Id") REFERENCES "User"("Id")
);

CREATE TABLE IF NOT EXISTS "GroupRole" (
    "Id"	INTEGER,
	"Rank"	INTEGER,
	"Created"	INTEGER,
	"Updated"	INTEGER,
    "FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	PRIMARY KEY("Id","Rank"),
	FOREIGN KEY("Id") REFERENCES "Group"("Id")
);

CREATE TABLE IF NOT EXISTS "GroupWall" (
    "Id"	INTEGER,
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

CREATE TABLE IF NOT EXISTS "GroupLog" (
    "Id"	INTEGER,
	"LogId"	INTEGER,
    "UserId"	INTEGER,
	"LogType"	INTEGER,
	"UserData"	TEXT,
	"Date"	INTEGER,
    "FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	PRIMARY KEY("Id","LogId"),
    FOREIGN KEY("UserId") REFERENCES "User"("Id"),
	FOREIGN KEY("Id") REFERENCES "Group"("Id")
);

CREATE TABLE IF NOT EXISTS "GroupAlly" (
    "Id"	INTEGER,
	"AllyId"	INTEGER,
	"AlliedSince"	INTEGER,
	"UnalliedSince"	INTEGER,
	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	PRIMARY KEY("Id","AllyId","AlliedSince"),
    FOREIGN KEY("Id") REFERENCES "Group"("Id"),
	FOREIGN KEY("AllyId") REFERENCES "Group"("Id")
);

CREATE TABLE IF NOT EXISTS "GroupEnemy" (
    "Id"	INTEGER,
	"EnemyId"	INTEGER,
	"AntagonizedSince"	INTEGER,
	"UnantagonizedSince"	INTEGER,
	"FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	PRIMARY KEY("Id","EnemyId","AntagonizedSince"),
    FOREIGN KEY("Id") REFERENCES "Group"("Id"),
	FOREIGN KEY("EnemyId") REFERENCES "Group"("Id")
);

CREATE TABLE IF NOT EXISTS "GroupHistorical" (
    "Id"	INTEGER,
    "MemberCount"	INTEGER,
    PRIMARY KEY("Id"),
	FOREIGN KEY("Id") REFERENCES "Group"("Id")
);

CREATE TABLE IF NOT EXISTS "GroupSocialLink" (
    "Id"	INTEGER,
    "LinkType"	INTEGER NOT NULL,
    "Url"	TEXT NOT NULL,
    "Title"	TEXT NOT NULL,
    PRIMARY KEY("Id"),
	FOREIGN KEY("Id") REFERENCES "Group"("Id")
);

CREATE TABLE IF NOT EXISTS "SetAsset" (
    "Id"	INTEGER,
	"AssetId"	INTEGER,
	PRIMARY KEY("Id","AssetId"),
	FOREIGN KEY("Id") REFERENCES "Set"("Id"),
	FOREIGN KEY("AssetId") REFERENCES "Asset"("Id")
);
)***";