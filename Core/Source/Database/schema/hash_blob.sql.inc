/**
 * The hash algorithm is SHA-256
 */
static const char *schema_hash_blob = R"***(
CREATE TABLE IF NOT EXISTS "HashBlob" (
    "Hash"	TEXT NOT NULL,
	"Blob"	BLOB NOT NULL,
	PRIMARY KEY("Hash")
);
)***";