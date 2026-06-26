-- ////////////////////////////////////////////////////////////////////////////////
-- noobWarrior
-- Plugin: Master Server
-- File: migrations.lua
-- Description:
-- Started by: Hattozo
-- Started on: 6/11/2026
-- ////////////////////////////////////////////////////////////////////////////////
local migrations = {}

migrations.list = {
    {
        version = "v1",
        sql = [[
CREATE TABLE IF NOT EXISTS WorkshopSubmission (
    Id               INTEGER PRIMARY KEY,
    UploaderId       INTEGER NOT NULL,
    Name             TEXT    NOT NULL,
    Description      TEXT,
    Type             TEXT    NOT NULL,
    Filename         TEXT,
    SizeBytes        INTEGER NOT NULL,
    Hash             TEXT    NOT NULL,
    CreatedTimestamp INTEGER NOT NULL DEFAULT (unixepoch())
);
]]
    },
    {
        -- Federation: peers + cross-server messages (inbound Message; OutboundMessage
        -- is kept so we can answer a recipient's origin callback).
        version = "v2",
        sql = [[
CREATE TABLE IF NOT EXISTS Peer (
    Id        INTEGER PRIMARY KEY,
    Domain    TEXT    NOT NULL UNIQUE,
    BaseUrl   TEXT    NOT NULL,
    Name      TEXT,
    FirstSeen INTEGER NOT NULL DEFAULT (unixepoch()),
    LastSeen  INTEGER NOT NULL DEFAULT (unixepoch()),
    Status    TEXT    NOT NULL DEFAULT 'active'
);

CREATE TABLE IF NOT EXISTS Message (
    Id               INTEGER PRIMARY KEY,
    FromIdentity     TEXT    NOT NULL,
    ToUserId         INTEGER NOT NULL,
    ToUsername       TEXT    NOT NULL,
    Body             TEXT    NOT NULL,
    ActionId         TEXT,
    Verified         INTEGER NOT NULL DEFAULT 0,
    ReadFlag         INTEGER NOT NULL DEFAULT 0,
    CreatedTimestamp INTEGER NOT NULL DEFAULT (unixepoch())
);
CREATE INDEX IF NOT EXISTS idx_message_to ON Message(ToUserId);
CREATE UNIQUE INDEX IF NOT EXISTS idx_message_action ON Message(ActionId) WHERE ActionId IS NOT NULL;

CREATE TABLE IF NOT EXISTS OutboundMessage (
    Id               INTEGER PRIMARY KEY,
    ActionId         TEXT    NOT NULL UNIQUE,
    FromUserId       INTEGER NOT NULL,
    FromUsername     TEXT    NOT NULL,
    ToIdentity       TEXT    NOT NULL,
    Body             TEXT    NOT NULL,
    BodyHash         TEXT    NOT NULL,
    CreatedTimestamp INTEGER NOT NULL DEFAULT (unixepoch())
);
]]
    },
    {
        -- Dedup for inbound federated actions not keyed by an action id elsewhere (forum posts).
        version = "v3",
        sql = [[
CREATE TABLE IF NOT EXISTS ReceivedAction (
    ActionId   TEXT PRIMARY KEY,
    Kind       TEXT,
    ReceivedAt INTEGER NOT NULL DEFAULT (unixepoch())
);
]]
    },
    {
        -- "My Feed" statuses: top-level (ParentId NULL) + replies, keyed by full identity.
        version = "v4",
        sql = [[
CREATE TABLE IF NOT EXISTS Status (
    Id             INTEGER PRIMARY KEY,
    AuthorIdentity TEXT    NOT NULL,
    Body           TEXT    NOT NULL,
    ParentId       INTEGER,
    ActionId       TEXT,
    Created        INTEGER NOT NULL DEFAULT (unixepoch())
);
CREATE INDEX IF NOT EXISTS idx_status_parent ON Status(ParentId);
CREATE UNIQUE INDEX IF NOT EXISTS idx_status_action ON Status(ActionId) WHERE ActionId IS NOT NULL;
]]
    },
    {
        version = "v5",
        sql = [[
ALTER TABLE WorkshopSubmission ADD COLUMN ThumbnailHash TEXT;
ALTER TABLE WorkshopSubmission ADD COLUMN ThumbnailMime TEXT;
]]
    },
    {
        version = "v6",
        sql = [[
CREATE TABLE IF NOT EXISTS WorkshopComment (
    Id               INTEGER PRIMARY KEY,
    SubmissionId     INTEGER NOT NULL,
    AuthorId         INTEGER NOT NULL,
    Body             TEXT    NOT NULL,
    CreatedTimestamp INTEGER NOT NULL DEFAULT (unixepoch())
);
CREATE INDEX IF NOT EXISTS idx_workshopcomment_submission ON WorkshopComment(SubmissionId);
]]
    },
}

local SCHEMA_MIGRATION = [[
CREATE TABLE IF NOT EXISTS Migration (
    RowId             INTEGER PRIMARY KEY,
    Version           TEXT    NOT NULL UNIQUE,
    AppliedTimestamp  INTEGER NOT NULL DEFAULT (unixepoch())
);
]]

local function VerifyIntegrity(db)
    local rows = db:QueryTyped("SELECT RowId, Version FROM Migration ORDER BY RowId ASC;")
    if rows == false then
        return false, "could not select from Migration table (it most likely does not exist)"
    end
    if rows == nil then
        return true -- no migrations recorded yet, nothing to verify
    end

    local prevRowId, prevVer = 0, 0
    for _, row in ipairs(rows) do
        local rowId = row.RowId
        local version = row.Version

        -- Cut the leading "v" off "v1" so it becomes a plain number
        local num = version
        if type(version) == "string" and version:sub(1, 1) == "v" then
            num = version:sub(2)
        end
        num = tonumber(num)

        if num == nil then
            return false, string.format("cannot convert version string \"%s\" to a number", tostring(version))
        end
        if num ~= rowId then
            return false, string.format(
                "version %s does not match row ID %d. Did the developer order the versions wrong? Is there a gap?",
                tostring(version), rowId)
        end
        if rowId > prevRowId and prevVer > num then
            return false, string.format(
                "newer version %s has a lower number than previous version %d. Did the developer order the versions wrong?",
                tostring(version), prevVer)
        end

        prevRowId, prevVer = rowId, num
    end
    return true
end

function migrations.MigrateToLatestVersion(db, list)
    list = list or migrations.list

    if db == nil then
        return false, "no database provided"
    end

    if not db:ExecStatement("BEGIN TRANSACTION;") then
        return false, "failed to begin migration transaction"
    end

    local function fail(msg)
        db:ExecStatement("ROLLBACK;")
        return false, msg
    end

    if not db:ExecStatement(SCHEMA_MIGRATION) then
        return fail("failed to create Migration table")
    end

    local ok, msg = VerifyIntegrity(db)
    if not ok then
        return fail("integrity check failed before migrating: " .. msg)
    end

    for _, migration in ipairs(list) do
        local existing = db:QueryTyped("SELECT RowId FROM Migration WHERE Version = ?;", migration.version)
        if existing == false then
            return fail("failed to query Migration table for " .. tostring(migration.version))
        end

        if existing == nil then
            if not db:ExecStatement(migration.sql) then
                return fail(string.format("migration to %s failed", tostring(migration.version)))
            end
            local recorded = db:QueryTyped("INSERT INTO Migration (Version) VALUES (?);", migration.version)
            if recorded == false then
                return fail(string.format("failed to record %s in the Migration table", tostring(migration.version)))
            end
            print(string.format("[MasterServer] Migrated database to %s", tostring(migration.version)))
        end
    end

    ok, msg = VerifyIntegrity(db)
    if not ok then
        return fail("integrity check failed after migrating: " .. msg)
    end

    if not db:ExecStatement("COMMIT;") then
        return fail("failed to commit migration transaction")
    end
    return true
end

return migrations
