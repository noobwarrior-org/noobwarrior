static const char *schema_user_friends = R"***(
CREATE TABLE IF NOT EXISTS "UserFriends" (
    "UserId"	INTEGER NOT NULL,
	"FriendId"	INTEGER NOT NULL,
	"FriendedSince"	INTEGER,
	"UnfriendedSince"	INTEGER,
    "FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
	PRIMARY KEY("UserId","FriendId","FriendedSince"),
	FOREIGN KEY("FriendId") REFERENCES "User"("Id"),
	FOREIGN KEY("UserId") REFERENCES "User"("Id")
);
)***";