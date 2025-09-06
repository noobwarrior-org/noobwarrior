/**
 * The hash algorithm is SHA-256
 */
static const char *schema_blob_hash = R"***(
CREATE TABLE IF NOT EXISTS "BlobHash" (
    "Hash"	TEXT NOT NULL,
	"Blob"	BLOB NOT NULL,
	PRIMARY KEY("Hash")
);
)***";