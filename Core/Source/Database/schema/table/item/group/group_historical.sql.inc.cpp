/**
 * This represents historical data and is not meant to be updated in a live database.
 * All real-time data is interpolated with the data in this table.
 */
static const char *schema_group_historical = R"***(
CREATE TABLE IF NOT EXISTS "GroupHistorical" (
    "Id"	INTEGER NOT NULL,
	"Snapshot"	INTEGER NOT NULL,
    "MemberCount"	INTEGER,
    PRIMARY KEY("Id","Snapshot"),
	FOREIGN KEY("Id", "Snapshot") REFERENCES "Group"("Id", "Snapshot")
);
)***";