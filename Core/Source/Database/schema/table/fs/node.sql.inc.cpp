static const char *schema_fs_node = R"***(
CREATE TABLE IF NOT EXISTS "FsNode" (
    "Id"	INTEGER PRIMARY KEY AUTOINCREMENT,
	"ParentId" INTEGER,
    "Name" TEXT NOT NULL,
    "Type"  INTEGER NOT NULL,
    "Content"   BLOB
);
)***";