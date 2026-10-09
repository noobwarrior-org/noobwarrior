# EmuDb
An EmuDb is a `.nwdb` file: an SQLite database of Roblox items in the shape the emulator serves them. The class is `EmuDb` in `Core/Source/EmuDb/EmuDb.cpp`, a subclass of the plain SQLite wrapper `SqlDb`. A short description of the format sits at the top of `EmuDb.h`.

# Tables
Each [item type](/api/luau/enums/ItemType) has a table named after it: `Asset`, `Badge`, `Bundle`, `DevProduct`, `Group`, `Outfit`, `Pass`, `Set`, `Universe` and `User`. Their extra data lives in tables that share the prefix, such as `AssetData` (an asset's content, by version), `AssetHistorical`, `UniversePlace` and `UserInventory`.

A few tables aren't items:

| Table | Holds |
| --- | --- |
| `Meta` | Key and value pairs describing the database itself: title, description, version, author, icon, compression and `Mutable`. |
| `BlobStorage` | Every piece of binary content, keyed by its SHA-256 hash. |
| `Migration` | The name of every migration applied to the file. |
| `LoginSession`, `AuthTicket` | Login sessions and one-time join tickets. Only the master database uses them. |
| `FsNode` | Files stored inside the database. |

`Meta` also has `OnlyEnableIfServerWithPlaceFromThisDatabaseIsRunning` and `TakeHigherPriorityIfServerWithPlaceFromThisDatabaseIsRunning`, seeded by the first migration. Nothing reads them yet.

# Migrations
The schema is built by migrations in `Core/Source/EmuDb/migrations/`, from `v1` upwards. Opening a database runs every migration missing from its `Migration` table, in order, and records each one. They are not compiled on their own: each `vN.sql.inc.cpp` file defines a string that `EmuDb.cpp` includes.

To change the schema:

1. Create `migrations/vN.sql.inc.cpp`, where `N` is one more than the highest existing number, defining `static const char *migration_vN = R"***( ... )***";`.
2. `#include` it in `EmuDb.cpp`.
3. Add `MIGRATE(vN)` at the end of the list in `MigrateToLatestVersion()`.
4. Change `kLatestMigrationName`, just below that list, to `"vN"`.

Never edit a migration that has shipped. Files already have it recorded and won't run it again.

Step 4 matters because `IsSchemaUpToDate()` compares the file against `kLatestMigrationName`, and a read-only database that fails that check is refused. `GetMigrationVersion()` reads SQLite's `user_version`, which nothing sets. It doesn't tell you anything.

Foreign keys are enforced from migration `v4` onwards. Adding a column with `ALTER TABLE ... ADD COLUMN` is safe. Deleting across tables has to follow foreign key order, which `DeleteItem` handles.

# Read-only files and the Mutable flag
Two separate things decide whether a database can change.

The first is how the file was opened. `SqlDb::OpenMode::ReadOnly` opens with `SQLITE_OPEN_READONLY` and never creates the file. A read-only connection can't run migrations, so an outdated database opened read-only fails with `FailReason::ReadOnlyOutOfDate`.

The second is the `Mutable` flag in `Meta`, which the database editor's Overview tab changes. It says whether the database accepts changes while the emulator runs, for example a player buying an item. The editor opens its own read-write connection, so turning the flag off can't lock anyone out of turning it back on.

`EmuDb::AllowsRuntimeWrites()` combines the two, and handlers should check it before writing. When mounting, the manager calls `EmuDb::ProbeIsMutable()` to read the flag on a throwaway connection, then opens the database read-only if the flag is off.

`AssetEnricher` is the exception to watch for. It fills in details for assets saved by asset grab mode on a background thread, using its own read-write connection to each file, which a read-only mount doesn't restrict. Any new background writer has to check `Mutable` itself.

# Mounting
`EmuDbManager` holds the mounted databases in a list. The list order is the priority order: lookups go through it from the start, and the first database with the item wins.

Databases are mounted during startup in this order:

1. `MountDatabases()` mounts each entry of the `databases.mounted` registry key. A plain path is opened directly. An entry with a URL scheme is a database a plugin offers, and goes to `PluginManager::MountDeclaredDatabase()`.
2. `MountMasterDbIfNotAlreadyMounted()` creates or opens `<userdata>/databases/master.nwdb` if nothing was mounted, and puts it first.
3. Plugins' required databases are mounted after the user's.

The order matters because `GetMasterDatabase()` returns the first database in the list. That database holds users, login sessions and forum posts, so plugin databases must never land in front of it.

## The scratch database
`GetTemporaryDatabase()` creates an in-memory database on first use. It holds items fetched from another server while joining it, which shouldn't be saved anywhere. It is searched after every mounted database, and `UnmountDatabases()` clears it.

## Writing
`GetWritableDbForItem()` finds the database to write an item to. It returns the database that has the item if that database allows runtime writes, and otherwise the master database. A read-only database's item is overlaid by a copy in the master, not changed in place.

# Blobs
All binary content goes through `AddBlob()`. It hashes the data with SHA-256 and stores it in `BlobStorage` under that hash. If the hash is already there, it stores nothing and returns `DidNothing`. When the database's compression is set to Zstandard, the data is compressed at level 3 before it's stored. Data of 2 GiB or more is refused with `BlobTooLarge`.

Because identical content is stored once, many assets can point at the same blob. Blobs have no reference count. When something that uses a blob is deleted, `GarbageCollectBlobIfOrphaned()` checks whether anything still refers to the hash and removes the blob if nothing does. `GarbageCollectOrphanedBlobs()` sweeps the whole database.

# Deleting items
The schema has no `ON DELETE CASCADE`, so `DeleteItem()` does the cascading in code. It reads the live schema with `PRAGMA foreign_key_list` to find every table that refers to the item, deletes those rows first, then deletes the item and collects any blobs left without a user. Because it reads the schema, it keeps working as later migrations add tables.

Cascading in the schema was rejected because blobs are shared between items. A schema cascade would delete content still used by something else.
