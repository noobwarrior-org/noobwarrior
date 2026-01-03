static const char *schema_user_friends = R"***(
CREATE TABLE IF NOT EXISTS "UserFriends" (
    "Id"	INTEGER NOT NULL,
	"FriendId"	INTEGER NOT NULL,
	"FriendedSince"	INTEGER,
	"UnfriendedSince"	INTEGER,
    "FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	PRIMARY KEY("Id","FriendId","FriendedSince"),
	FOREIGN KEY("FriendId") REFERENCES "User"("Id"),
	FOREIGN KEY("Id") REFERENCES "User"("Id")
);
)***";