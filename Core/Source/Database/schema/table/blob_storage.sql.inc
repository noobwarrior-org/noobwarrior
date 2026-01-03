/**
 * All binary blobs for multimedia should be stored here, with their corresponding hashed value as the key.
 * The hash algorithm is SHA-256
 */
static const char *schema_blob_storage = R"***(
CREATE TABLE IF NOT EXISTS "BlobStorage" (
    "Hash"	TEXT NOT NULL,
	"Blob"	BLOB NOT NULL,
	PRIMARY KEY("Hash")
);
)***";