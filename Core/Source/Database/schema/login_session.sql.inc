static const char *schema_login_session = R"***(
CREATE TABLE IF NOT EXISTS "LoginSession" (
    "Token"	TEXT NOT NULL UNIQUE,
    "CreationTimestamp"	INTEGER DEFAULT (unixepoch()),
    "LastUsedTimestamp"	INTEGER DEFAULT (unixepoch()),
	"UserId"	INTEGER NOT NULL,
    "Location"     TEXT,
    "Device"     TEXT,
	PRIMARY KEY("Token"),
    FOREIGN KEY("UserId") REFERENCES "User"("Id")
);
)***";