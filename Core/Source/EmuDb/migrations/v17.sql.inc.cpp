/*
 * Copyright (C) 2026 Hattozo
 *
 * This file is part of noobWarrior.
 *
 * noobWarrior is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * noobWarrior is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with noobWarrior; if not, see
 * <https://www.gnu.org/licenses/>.
 */
static const char *migration_v17 = R"***(
-- DataStoreService persistence.
--
-- Models how Roblox's in-game DataStore actually works under the hood (the
-- gamepersistence.roblox.com /v2/persistence and legacy /persistence wire APIs):
--
--   * A DataStore is identified by (universe, name, type). Type 0 is a standard
--     GlobalDataStore/DataStore, type 1 is an OrderedDataStore. GetGlobalDataStore()
--     is just the standard datastore whose name is the reserved value the engine
--     sends for it. Datastores are created implicitly on first write; the registry
--     row also remembers per-store config (whether versioning is enabled).
--
--   * An object (a key) is identified by (universe, datastore, scope, key, type).
--     Scope defaults to "global"; the v2 wire format carries it as the "scope/key"
--     objectKey. DataStoreEntry is the *current* pointer for an object: its latest
--     version, its first-created/last-updated times, a tombstone flag set by
--     RemoveAsync, and a denormalized numeric SortValue used for OrderedDataStore
--     range queries (GetSortedAsync) and IncrementAsync.
--
--   * Every write is immutable and creates a new DataStoreVersion row (this is what
--     ListVersionsAsync/GetVersionAsync read). A version stores the raw JSON value
--     text exactly as SetAsync sent it, its length and base64(md5(value)) checksum
--     (the content-md5 the engine validates), the attached UserIds and metadata
--     Attributes (DataStoreKeyInfo:GetUserIds/GetMetadata), and a Deleted flag for
--     the tombstone version RemoveAsync appends. RemoveVersionAsync deletes a row.
--
-- Values are stored as TEXT because a DataStore value is always a JSON document
-- (the string the engine JSON-decodes on GetAsync); a delete tombstone stores NULL.

CREATE TABLE IF NOT EXISTS DataStore (
    Id                  INTEGER PRIMARY KEY AUTOINCREMENT,
    UniverseId          INTEGER NOT NULL,
    Name                TEXT NOT NULL,
    Type                INTEGER NOT NULL DEFAULT 0,   -- 0 = Standard, 1 = Ordered
    CreatedTime         INTEGER NOT NULL DEFAULT 0,   -- milliseconds since the Unix epoch
    VersioningEnabled   INTEGER NOT NULL DEFAULT 1,
    UNIQUE(UniverseId, Name, Type)
);

CREATE TABLE IF NOT EXISTS DataStoreEntry (
    Id                  INTEGER PRIMARY KEY AUTOINCREMENT,
    UniverseId          INTEGER NOT NULL,
    DataStoreName       TEXT NOT NULL,
    Scope               TEXT NOT NULL DEFAULT 'global',
    EntryKey            TEXT NOT NULL,                -- the user's key within (datastore, scope)
    Type                INTEGER NOT NULL DEFAULT 0,   -- mirrors DataStore.Type
    CurrentVersionId    TEXT,                         -- DataStoreVersion.VersionId of the live value
    SortValue           INTEGER,                      -- numeric value for ordered stores / range queries; NULL otherwise
    Deleted             INTEGER NOT NULL DEFAULT 0,   -- 1 when the current version is a RemoveAsync tombstone
    CreatedTime         INTEGER NOT NULL DEFAULT 0,   -- milliseconds; when the key was first created
    UpdatedTime         INTEGER NOT NULL DEFAULT 0,   -- milliseconds; last successful write
    UNIQUE(UniverseId, DataStoreName, Scope, EntryKey, Type)
);

CREATE TABLE IF NOT EXISTS DataStoreVersion (
    Id                  INTEGER PRIMARY KEY AUTOINCREMENT,
    EntryId             INTEGER NOT NULL,
    VersionId           TEXT NOT NULL,                -- opaque, lexicographically time-sortable id
    Value               TEXT,                         -- raw JSON value text; NULL for a tombstone
    ContentLength       INTEGER NOT NULL DEFAULT 0,   -- byte length of Value
    ContentMd5          TEXT,                         -- base64(md5(Value)), the content-md5 the engine checks
    UserIds             TEXT NOT NULL DEFAULT '[]',   -- JSON array of associated user ids
    Attributes          TEXT NOT NULL DEFAULT '{}',   -- JSON object of metadata (DataStoreSetOptions:SetMetadata)
    Deleted             INTEGER NOT NULL DEFAULT 0,   -- 1 when this version is a tombstone (RemoveAsync)
    CreatedTime         INTEGER NOT NULL DEFAULT 0,   -- milliseconds since the Unix epoch
    UNIQUE(EntryId, VersionId),
    FOREIGN KEY(EntryId) REFERENCES DataStoreEntry(Id)
);

CREATE INDEX IF NOT EXISTS idx_datastoreentry_lookup
    ON DataStoreEntry(UniverseId, DataStoreName, Scope, Type, EntryKey);

CREATE INDEX IF NOT EXISTS idx_datastoreentry_sort
    ON DataStoreEntry(UniverseId, DataStoreName, Scope, Type, SortValue);

CREATE INDEX IF NOT EXISTS idx_datastoreversion_entry
    ON DataStoreVersion(EntryId, VersionId);
)***";
