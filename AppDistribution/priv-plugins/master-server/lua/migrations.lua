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
