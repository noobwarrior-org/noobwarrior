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

reg.SetKeyValueIfNotSet("master.domain", "localhost")
reg.SetKeyComment("master.domain", "This master server's domain name. Online user identities are formed as username@<domain> (e.g. alice@example.com). Set this to the public hostname other master servers and clients reach you at.")

reg.SetKeyValueIfNotSet("master.public_url", "")
reg.SetKeyComment("master.public_url", "This server's public base URL that other master servers reach you at (e.g. https://example.com). If blank, https://<master.domain> is assumed.")

reg.SetKeyValueIfNotSet("master.online_id_32bit", true)
reg.SetKeyComment("master.online_id_32bit", "Keep online user ids inside a signed 32-bit integer so older Roblox clients that store user ids as 32-bit don't overflow.")

reg.SetKeyValueIfNotSet("master.federation.auto", true)
reg.SetKeyComment("master.federation.auto", "When true, federation is automatic: any server that contacts you is accepted as a peer, and peers-of-peers are discovered as your feed refreshes. Turn off (in the control panel's Federation tab) to only federate with peers you add by hand.")

reg.SetKeyValueIfNotSet("master.federation.banned", {})
reg.SetKeyComment("master.federation.banned", "List of defederated (banned) master server domains. Banned masters cannot federate, message, post, or have their users join servers that trust you. Managed from the control panel's Federation tab.")

reg.SetKeyValueIfNotSet("master.auth.require_for_hosting", true)
reg.SetKeyComment("master.auth.require_for_hosting", "If true, a server emulator must be signed in to an account on this master server before it may announce itself and appear in the server list. Turn off to let anyone list a server here.")

reg.SetKeyValueIfNotSet("master.auth.require_for_joining", false)
reg.SetKeyComment("master.auth.require_for_joining", "If true, only signed-in accounts may use this master server as their identity when joining a game server that trusts it. Turn off to let people join through this master without an account (the game server still decides whether it admits guests).")

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

function _G.MASTERSERVER_FIRST_ROW(result)
    return type(result) == "table" and result[1] or nil
end

function _G.MASTERSERVER_BLANK(s)
    return s == nil or tostring(s):match("^%s*$") ~= nil
end

function _G.MASTERSERVER_DOMAIN()
    local d = reg.GetKeyValue("master.domain")
    return _G.MASTERSERVER_BLANK(d) and "localhost" or tostring(d)
end

function _G.MASTERSERVER_FULL_IDENTITY(username, domain)
    return tostring(username) .. "@" .. (domain or _G.MASTERSERVER_DOMAIN())
end

-- "/@name" for a local identity, "/@name@domain" for a remote one.
function _G.MASTERSERVER_PROFILE_URL(identity)
    identity = tostring(identity or "")
    local uname, domain = identity:match("^(.-)@([^@]+)$")
    if uname and domain and domain:lower() == _G.MASTERSERVER_DOMAIN():lower() then
        return "/@" .. uname
    end
    return "/@" .. identity
end

function _G.MASTERSERVER_ONLINE_USER_ID(identifier)
    -- Master servers have no public-facing user id; the id is a hash of the full handle "@user@domain".
    local handle = tostring(identifier)
    if handle:sub(1, 1) ~= "@" then handle = "@" .. handle end
    -- 52 bits of the SHA-256 (exact in a double). Build it from two <=32-bit halves: LuaJIT's
    -- tonumber(hex, 16) SATURATES at 0xFFFFFFFF for >32-bit values, so tonumber(sub(1,13),16) would
    -- collapse almost every handle to the same id.
    local hex = hash.Sha256(handle)
    local hi = tonumber(hex:sub(1, 5), 16) or 0    -- 20 bits
    local lo = tonumber(hex:sub(6, 13), 16) or 0   -- 32 bits
    local n = hi * 4294967296 + lo                 -- 52-bit value
    local floor = 1000000000
    -- Default to a signed-32-bit-safe ceiling for older clients; the wider 53-bit space is opt-in.
    local ceiling = (reg.GetKeyValue("master.online_id_32bit") == false) and (2 ^ 53) or (2 ^ 31)
    return floor + (n % (ceiling - floor))
end

function _G.MASTERSERVER_HTML_ESCAPE(s)
    return (tostring(s or ""):gsub("[&<>\"']", {
        ["&"] = "&amp;", ["<"] = "&lt;", [">"] = "&gt;", ["\""] = "&quot;", ["'"] = "&#39;",
    }))
end

function _G.MASTERSERVER_URL_ENCODE(s)
    return (tostring(s or ""):gsub("[^%w%-_%.~]", function(c)
        return string.format("%%%02X", string.byte(c))
    end))
end

function _G.MASTERSERVER_HUMAN_SIZE(bytes)
    bytes = tonumber(bytes) or 0
    local units, i = { "B", "KB", "MB", "GB", "TB" }, 1
    while bytes >= 1024 and i < #units do
        bytes = bytes / 1024
        i = i + 1
    end
    return string.format(i == 1 and "%d %s" or "%.1f %s", bytes, units[i])
end

-- Looks up an item's name in whichever mounted EmuDb holds it
local function resolveItemName(itemType, tableName, id)
    id = tonumber(id)
    if not id then return nil end
    local ok, name = pcall(function()
        local mgr = core.GetEmuDbManager()
        local db = mgr and mgr:GetFirstDbWhereItemExists(itemType, id)
        local row = db and _G.MASTERSERVER_FIRST_ROW(db:QueryTyped("SELECT Name FROM " .. tableName .. " WHERE Id = ?;", id))
        return row and row.Name
    end)
    return (ok and not _G.MASTERSERVER_BLANK(name)) and tostring(name) or nil
end

function _G.MASTERSERVER_RESOLVE_UPLOADER_NAME(uploaderId)
    return resolveItemName(ItemType.User, "User", uploaderId) or "Unknown"
end

function _G.MASTERSERVER_RESOLVE_PLACE_NAME(placeId)
    if not tonumber(placeId) then return "Unknown Game" end
    return resolveItemName(ItemType.Asset, "Asset", placeId) or ("Place " .. tostring(tonumber(placeId)))
end

_G.EMU_SERVERS = {}

local EMU_SERVER_TIMEOUT = 30 -- seconds before a server we haven't heard from is swept

function _G.MASTERSERVER_SWEEP_EMU_SERVERS()
    local now = os.time()
    for key, srv in pairs(_G.EMU_SERVERS) do
        if now - srv.LastSeen > EMU_SERVER_TIMEOUT then
            _G.EMU_SERVERS[key] = nil
        end
    end
end

_G.WORKSHOP_UPLOADS = {} -- in-progress upload sessions
local WORKSHOP_UPLOAD_TIMEOUT = 600  -- seconds of inactivity before a session is swept

function _G.MASTERSERVER_WORKSHOP_MAX_UPLOAD_BYTES()
    local mb = tonumber(reg.GetKeyValue("master.workshop.max_upload_mb")) or 4096
    return math.floor(mb * 1024 * 1024)
end

-- Workshop thumbnails are small images stored alongside submissions
function _G.MASTERSERVER_WORKSHOP_MAX_THUMBNAIL_BYTES()
    return 8 * 1024 * 1024 -- 8 MiB is plenty for a thumbnail
end

_G.MASTERSERVER_WORKSHOP_THUMBNAIL_MIMES = {
    ["image/png"]  = true,
    ["image/jpeg"] = true,
    ["image/gif"]  = true,
    ["image/webp"] = true,
}

function _G.MASTERSERVER_SWEEP_WORKSHOP_UPLOADS()
    local now = os.time()
    for id, up in pairs(_G.WORKSHOP_UPLOADS) do
        if now - up.LastActivity > WORKSHOP_UPLOAD_TIMEOUT then
            _G.WORKSHOP_UPLOADS[id] = nil
        end
    end
end

function _G.MASTERSERVER_RESOLVE_SESSION_USER(token)
    if not token or token:match("^%s*$") then return nil end
    local db = core.GetMasterDatabase()
    if db == nil then return nil end
    -- Reject sessions idle past the TTL (0 = never expire), matching the native resolver.
    local ttl = (tonumber(reg.GetKeyValue("emu.auth.session_ttl_days")) or 30) * 86400
    local rows = db:QueryTyped(
        "SELECT u.Id AS Id, u.Name AS Name, u.DisplayName AS DisplayName " ..
        "FROM LoginSession s INNER JOIN User u ON u.Id = s.UserId " ..
        "WHERE s.Token = ? AND (? <= 0 OR (unixepoch() - s.LastUsedTimestamp) < ?);",
        token, ttl, ttl
    )
    if rows == nil or rows[1] == nil then return nil end
    return rows[1]
end

local migrations = require("plugin://master-server@noobwarrior.org/lua/migrations.lua")
local migrateOk, migrateErr = migrations.MigrateToLatestVersion(_G.MASTERSERVER_DB)
if not migrateOk then
    print("Failed to migrate master server database: " .. tostring(migrateErr))
end

-- This master's Ed25519 federation keypair (hex). Peers pin our public key and verify our signed
-- actions against it. Generated once and persisted in the plugin's data dir.
local function loadOrGenerateFederationKeypair()
    local vfs = core.GetPluginDataDir()
    if not vfs then
        print("[master] no plugin data dir; cannot persist a federation keypair")
        return nil, nil
    end
    local privPath, pubPath = "master/federation.priv", "master/federation.pub"
    local function readAll(path)
        if not vfs:EntryExists(path) then return nil end
        local h = vfs:OpenHandle(path)
        local _, data = vfs:ReadHandleChunk(h, 256)
        vfs:CloseHandle(h)
        return (tostring(data or ""):gsub("%s+$", ""))
    end
    local priv, pub = readAll(privPath), readAll(pubPath)
    if priv and pub and #priv == 64 and #pub == 64 then
        return priv, pub
    end
    local kp = crypto.GenerateEd25519()
    if not kp then
        print("[master] FAILED to generate a federation keypair")
        return nil, nil
    end
    vfs:CreateDirectories("master")
    vfs:WriteFile(privPath, kp.priv)
    vfs:WriteFile(pubPath, kp.pub)
    print("[master] generated a new federation keypair (public key " .. kp.pub:sub(1, 16) .. "...)")
    return kp.priv, kp.pub
end
_G.MASTERSERVER_PRIVKEY, _G.MASTERSERVER_PUBKEY = loadOrGenerateFederationKeypair()

_G.MASTERSERVER_FEDERATION = require("plugin://master-server@noobwarrior.org/lua/federation.lua")

function _G.MASTERSERVER_DELETE_WORKSHOP_SUBMISSION(id)
    local db = _G.MASTERSERVER_DB
    if db == nil then return false, "No master server database available" end

    local rows = db:QueryTyped("SELECT Hash, ThumbnailHash FROM WorkshopSubmission WHERE Id = ?;", id)
    if rows == nil or rows[1] == nil then return false, "Submission not found" end
    local hashVal = rows[1].Hash
    local thumbHash = rows[1].ThumbnailHash

    db:QueryTyped("DELETE FROM WorkshopSubmission WHERE Id = ?;", id)
    db:QueryTyped("DELETE FROM WorkshopComment WHERE SubmissionId = ?;", id)

    local vfs = url.GetVfs(_G.MASTERSERVER_PLUGINDATA .. "/")

    -- Blobs are content-addressed and shared, so only delete one once no other
    -- submission still references its hash.
    local stillReferenced = db:QueryTyped("SELECT 1 FROM WorkshopSubmission WHERE Hash = ? LIMIT 1;", hashVal)
    if stillReferenced == nil or stillReferenced[1] == nil then
        local path = "/master/submissions/" .. hashVal .. ".nwdb"
        if vfs ~= nil and vfs:EntryExists(path) then
            vfs:DeleteEntry(path)
        end
    end

    if thumbHash ~= nil and not _G.MASTERSERVER_BLANK(thumbHash) then
        local thumbStillUsed = db:QueryTyped("SELECT 1 FROM WorkshopSubmission WHERE ThumbnailHash = ? LIMIT 1;", thumbHash)
        if thumbStillUsed == nil or thumbStillUsed[1] == nil then
            local thumbPath = "/master/thumbnails/" .. thumbHash
            if vfs ~= nil and vfs:EntryExists(thumbPath) then
                vfs:DeleteEntry(thumbPath)
            end
        end
    end
    return true
end

local http_base = require("plugin://http-base@noobwarrior.org/lua/base.lua")

local sitemap = {
    ["/"] = "/src/index.lhp",
    ["/home"] = "/src/index.lhp",
    ["/servers"] = "/src/servers.lhp",
    ["/join"] = "/src/join.lhp",
    ["/login"] = "/src/login.lhp",
    ["/register"] = "/src/register.lhp",
    ["/workshop"] = "/src/workshop.lhp",
    ["/messages"] = "/src/messages.lhp",
    ["/forums"] = "/src/forums.lhp",
    ["/forums/new-thread"] = "/src/forums_newthread.lhp",
    ["/feed"] = "/src/feed.lhp",
    ["/control-panel"] = "/src/controlpanel.lhp",
    ["/profile"] = "/src/profile.lhp",
    ["/@:username"] = "/src/userprofile.lhp",
    ["/users/:userId/profile"] = "/src/userprofile.lhp",

    -- API
    ["/v1/login"] = "/src/api/login.lhp",
    ["/emu/v1/create-account"] = "/src/api/create_account.lhp",
    ["/emu/v1/logout"] = "/src/api/logout.lhp",
    ["/v1/servers"] = "/src/api/servers.lhp",
    ["/v1/profile"] = "/src/api/profile.lhp",
    ["/v1/users/:username"] = "/src/api/user.lhp",
    ["/v1/emu-ping"] = "/src/api/emu-ping.lhp",
    ["/v1/messages/send"] = "/src/api/messages/send.lhp",
    ["/v1/forums/post"] = "/src/api/forums/post.lhp",
    ["/v1/feed/post"] = "/src/api/feed/post.lhp",
    ["/v1/federation/add-peer"] = { Page = "/src/api/federation/add_peer.lhp", Permission = "federation.manage" },
    ["/v1/federation/set-auto"] = { Page = "/src/api/federation/set_auto.lhp", Permission = "federation.manage" },
    ["/v1/federation/ban-peer"] = { Page = "/src/api/federation/ban_peer.lhp", Permission = "federation.manage" },
    ["/v1/federation/unban-peer"] = { Page = "/src/api/federation/unban_peer.lhp", Permission = "federation.manage" },
    ["/v1/join/mint-voucher"] = "/src/api/join/mint_voucher.lhp",
    ["/v1/join/verify-federated"] = "/src/api/join/verify_federated.lhp",
    ["/emu/v1/avatar/mine"] = "/src/api/avatar/mine.lhp",
    ["/emu/v1/avatar/catalog"] = "/src/api/avatar/catalog.lhp",
    ["/emu/v1/avatar/thumbnail"] = "/src/api/avatar/thumbnail.lhp",
    ["/v1/workshop/start-upload"] = "/src/api/workshop/start_upload.lhp",
    ["/v1/workshop/stream-upload"] = "/src/api/workshop/stream_upload.lhp",
    ["/v1/workshop/end-upload"] = "/src/api/workshop/end_upload.lhp",
    ["/v1/workshop/list"] = "/src/api/workshop/list.lhp",
    ["/v1/workshop/item"] = "/src/api/workshop/item.lhp",
    ["/v1/workshop/download"] = "/src/api/workshop/download.lhp",
    ["/v1/workshop/delete"] = "/src/api/workshop/delete.lhp",
    ["/v1/workshop/set-thumbnail"] = "/src/api/workshop/set_thumbnail.lhp",
    ["/v1/workshop/thumbnail"] = "/src/api/workshop/thumbnail.lhp",
    ["/v1/workshop/edit"] = "/src/api/workshop/edit.lhp",
    ["/v1/workshop/comment"] = "/src/api/workshop/comment.lhp",
    ["/v1/workshop/comment-delete"] = "/src/api/workshop/comment_delete.lhp",

    -- Federation protocol
    ["/fed/v1/info"] = "/src/api/fed/info.lhp",
    ["/fed/v1/users/:username"] = "/src/api/fed/user.lhp",
    ["/fed/v1/avatar"] = "/src/api/fed/avatar.lhp",
    ["/fed/v1/catalog"] = "/src/api/fed/catalog.lhp",
    ["/fed/v1/thumbnail"] = "/src/api/fed/thumbnail.lhp",
    ["/fed/v1/asset"] = "/src/api/fed/asset.lhp",
    ["/fed/v1/inbox"] = "/src/api/fed/inbox.lhp",
    ["/fed/v1/servers"] = "/src/api/servers.lhp",
    ["/fed/v1/forums"] = "/src/api/fed/forums.lhp",
    ["/fed/v1/forum-post"] = "/src/api/fed/forum_post.lhp",
    ["/fed/v1/peers"] = "/src/api/fed/peers.lhp",
    ["/fed/v1/statuses"] = "/src/api/fed/statuses.lhp",
    ["/fed/v1/status-reply"] = "/src/api/fed/status_reply.lhp"
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
