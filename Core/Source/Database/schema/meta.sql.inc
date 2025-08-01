static const char *schema_meta = R"***(
CREATE TABLE IF NOT EXISTS "Meta" (
    "Key"	TEXT NOT NULL UNIQUE,
	"Value"	TEXT,
	PRIMARY KEY("Key")
);
)***";