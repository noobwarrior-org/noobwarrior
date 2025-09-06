static const char *schema_user_following = R"***(
CREATE TABLE IF NOT EXISTS "UserFollowing" (
    "UserId"	INTEGER NOT NULL,
    "FollowingId"	INTEGER NOT NULL,
	"FollowingSince"	INTEGER,
	"UnfollowingSince"	INTEGER,
    "FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
    PRIMARY KEY("UserId","FollowingId","FollowingSince"),
    FOREIGN KEY("FollowingId") REFERENCES "User"("Id"),
    FOREIGN KEY("UserId") REFERENCES "User"("Id")
);
)***";