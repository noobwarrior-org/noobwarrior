-- ////////////////////////////////////////////////////////////////////////////////
-- noobWarrior
-- Plugin: Master Server
-- File: main.lua
-- Description: Main entrypoint for Master Server plugin
-- Started by: Hattozo
-- Started on: 1/3/2026
-- ////////////////////////////////////////////////////////////////////////////////
reg.SetKeyValueIfNotSet("master.autostart", false)
reg.SetKeyComment("master.autostart", "Set to true to automatically make the master server start listening on startup.")

reg.SetKeyValueIfNotSet("master.http_port", 80)
reg.SetKeyComment("master.http_port", "Port the master server HTTP webserver binds to.")

reg.SetKeyValueIfNotSet("master.https_port", 443)
reg.SetKeyComment("master.https_port", "Port the master server HTTPS webserver binds to.")

reg.SetKeyValueIfNotSet("master.branding.title", "noobWarrior Master Server")
reg.SetKeyValueIfNotSet("master.branding.icon", "/img/icon1024.png")
reg.SetKeyValueIfNotSet("master.branding.tagline", "My noobWarrior server")
reg.SetKeyComment("master.branding", "The branding that people will see when they connect to your master server.")

reg.SetKeyValueIfNotSet("master.workshop.max_upload_mb", 4096)
reg.SetKeyComment("master.workshop.max_upload_mb", "Maximum size in megabytes of a single .nwdb file uploaded to the workshop.")

core.ConsoleAdded:Connect(function(console)
    console:RegisterCommand("master", function(ctx)
        ctx:Reply("Hello from master server!")
    end, "Master server commands.")
end)

local createDirSuccess = core.GetPluginDataDir():CreateDirectories("master")
if not createDirSuccess then
    print("Failed to create directory \"master\" in plugindata")
end
_G.MASTERSERVER_PLUGINDATA = "plugindata://master"
_G.MASTERSERVER_DB = SqlDb.new(_G.MASTERSERVER_PLUGINDATA .. "/master.nwdb", "MasterServerDb")

_G.EMU_SERVERS = {}

local EMU_SERVER_TIMEOUT = 30 -- seconds before a server we haven't heard from is swept

function _G.SweepEmuServers()
    local now = os.time()
    for key, srv in pairs(_G.EMU_SERVERS) do
        if now - srv.LastSeen > EMU_SERVER_TIMEOUT then
            _G.EMU_SERVERS[key] = nil
        end
    end
end

_G.WORKSHOP_UPLOADS = {} -- in-progress upload sessions
local WORKSHOP_UPLOAD_TIMEOUT = 600  -- seconds of inactivity before a session is swept

-- Maximum size, in bytes, a single upload is allowed to reach.
function _G.WorkshopMaxUploadBytes()
    local mb = tonumber(reg.GetKeyValue("master.workshop.max_upload_mb")) or 4096
    return math.floor(mb * 1024 * 1024)
end

-- Drop upload sessions we haven't heard from in a while so abandoned uploads
-- don't leak memory.
function _G.SweepWorkshopUploads()
    local now = os.time()
    for id, up in pairs(_G.WORKSHOP_UPLOADS) do
        if now - up.LastActivity > WORKSHOP_UPLOAD_TIMEOUT then
            _G.WORKSHOP_UPLOADS[id] = nil
        end
    end
end

-- Resolve the signed-in user from a .LOGINSESSION token, or nil if the token is
-- missing/expired. Mirrors the lookup done in header.lhp.
function _G.ResolveSessionUser(token)
    if not token or token:match("^%s*$") then return nil end
    local db = core.GetMasterDatabase()
    if db == nil then return nil end
    local rows = db:QueryTyped(
        "SELECT u.Id AS Id, u.Name AS Name, u.DisplayName AS DisplayName " ..
        "FROM LoginSession s INNER JOIN User u ON u.Id = s.UserId WHERE s.Token = ?;",
        token
    )
    if rows == nil or rows[1] == nil then return nil end
    return rows[1]
end

function _G.EnsureWorkshopSchema(db)
    db:Query([[CREATE TABLE IF NOT EXISTS WorkshopSubmission (
    Id               INTEGER PRIMARY KEY,
    UploaderId       INTEGER NOT NULL,
    Name             TEXT    NOT NULL,
    Description      TEXT,
    Type             TEXT    NOT NULL,
    Filename         TEXT,
    SizeBytes        INTEGER NOT NULL,
    Hash             TEXT    NOT NULL,
    CreatedTimestamp INTEGER NOT NULL DEFAULT (unixepoch())
);]])
end

_G.EnsureWorkshopSchema(_G.MASTERSERVER_DB)

function _G.DeleteWorkshopSubmission(id)
    local db = _G.MASTERSERVER_DB
    if db == nil then return false, "No master server database available" end

    local rows = db:QueryTyped("SELECT Hash FROM WorkshopSubmission WHERE Id = ?;", id)
    if rows == nil or rows[1] == nil then return false, "Submission not found" end
    local hashVal = rows[1].Hash

    db:QueryTyped("DELETE FROM WorkshopSubmission WHERE Id = ?;", id)

    local stillReferenced = db:QueryTyped("SELECT 1 FROM WorkshopSubmission WHERE Hash = ? LIMIT 1;", hashVal)
    if stillReferenced == nil or stillReferenced[1] == nil then
        local vfs = url.GetVfs(_G.MASTERSERVER_PLUGINDATA .. "/")
        local path = "/submissions/" .. hashVal .. ".nwdb"
        if vfs ~= nil and vfs:EntryExists(path) then
            vfs:DeleteEntry(path)
        end
    end
    return true
end

local http_base = require("plugin://http-base@noobwarrior.org/lua/base.lua")

local sitemap = {
    ["/"] = "/src/index.lhp",
    ["/home"] = "/src/index.lhp",
    ["/login"] = "/src/login.lhp",
    ["/register"] = "/src/register.lhp",
    ["/workshop"] = "/src/workshop.lhp",
    ["/control-panel"] = "/src/controlpanel.lhp",
    ["/profile"] = "/src/profile.lhp",

    -- API
    ["/v1/login"] = "/src/api/login.lhp",
    ["/v1/create-account"] = "/src/api/create_account.lhp",
    ["/v1/logout"] = "/src/api/logout.lhp",
    ["/v1/servers"] = "/src/api/servers.lhp",
    ["/v1/emu-ping"] = "/src/api/emu-ping.lhp",
    ["/v1/workshop/start-upload"] = "/src/api/workshop/start_upload.lhp",
    ["/v1/workshop/stream-upload"] = "/src/api/workshop/stream_upload.lhp",
    ["/v1/workshop/end-upload"] = "/src/api/workshop/end_upload.lhp",
    ["/v1/workshop/download"] = "/src/api/workshop/download.lhp",
    ["/v1/workshop/delete"] = "/src/api/workshop/delete.lhp"
}

master = http_base.CreateServer({
    Name = "MasterServer",
    Sitemap = sitemap
})
master:MountVolume("/", "/static") -- also mount our own static directory alongside the shared http server's
if reg.GetKeyValue("master.autostart") then
    local http_port = reg.GetKeyValue("master.http_port") or 80
    local https_port = reg.GetKeyValue("master.https_port") or 443
    master:Start(http_port)
    master:StartSecure(https_port)
    print(string.format("Automatically started master server! HTTP server listening on port %d and HTTPS server listening on port %d", http_port, https_port))
end
