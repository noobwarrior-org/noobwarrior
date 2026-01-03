static const char *schema_user_followers = R"***(
CREATE TABLE IF NOT EXISTS "UserFollowers" (
    "Id"	INTEGER NOT NULL,
    "FollowerId"	INTEGER NOT NULL,
	"FollowedSince"	INTEGER,
	"UnfollowedSince"	INTEGER,
    "FirstRecorded"	INTEGER DEFAULT (unixepoch()),
	"LastRecorded"	INTEGER DEFAULT (unixepoch()),
    PRIMARY KEY("Id","FollowerId","FollowedSince"),
    FOREIGN KEY("FollowerId") REFERENCES "User"("Id"),
    FOREIGN KEY("Id") REFERENCES "User"("Id")
);
)***";