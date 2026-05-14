-- ////////////////////////////////////////////////////////////////////////////////
-- noobWarrior
-- Plugin: Server Emulator Frontend
-- File: main.lua
-- Description: Main entrypoint for the server emulator frontend
-- Started by: Hattozo
-- Started on: 1/3/2026
-- ////////////////////////////////////////////////////////////////////////////////
if emu == nil then
    error("Server emulator global \"emu\" is nil! This should not happen!")
end
_G.EMU_FRONTEND_VER = "0.1"
emu:MountVolume("/", "/static")

emu.PreStart:Connect(function(secure)
    print("Starting "..(secure and "HTTPS" or "HTTP").." server emulator frontend!")
end)

emu.PreStop:Connect(function(secure)
    print("Stopping "..(secure and "HTTPS" or "HTTP").." server emulator frontend!")
end)

local http_base = require("plugin://http-base@noobwarrior.org/lua/base.lua")

local sitemap = {
    ["/"] = "plugin://emu-frontend@noobwarrior.org/src/index.lhp",
    ["/login"] = "plugin://emu-frontend@noobwarrior.org/src/login.lhp",
    ["/register"] = "plugin://emu-frontend@noobwarrior.org/src/register.lhp",
    ["/servers"] = "plugin://emu-frontend@noobwarrior.org/src/servers.lhp",
    ["/control-panel"] = "plugin://emu-frontend@noobwarrior.org/src/controlpanel.lhp"
}

-- Resolves the .LOGINSESSION cookie to a user record so LHP pages can read _SESSION.User
-- without re-querying the database themselves. Also bumps LastUsedTimestamp on each hit so
-- "remember me" sessions stay alive while in use.
local function ResolveSession(cookies, session, req)
    local token = cookies[".LOGINSESSION"]
    if token == nil or token == "" or token == "deleted" then return end

    local db = core.GetMasterDatabase()
    if db == nil then return end

    local rows = db:QueryTyped(
        "SELECT u.Id AS Id, u.Name AS Name, u.DisplayName AS DisplayName " ..
        "FROM LoginSession s INNER JOIN User u ON u.Id = s.UserId " ..
        "WHERE s.Token = ?;",
        token
    )
    if rows == nil or rows[1] == nil then return end

    local row = rows[1]
    session.Token = token
    session.User = {
        Id = row.Id,
        Name = row.Name,
        DisplayName = row.DisplayName,
    }

    db:QueryTyped("UPDATE LoginSession SET LastUsedTimestamp = unixepoch() WHERE Token = ?;", token)
end

http_base.AttachToServer(emu, {
    Sitemap = sitemap,
    ResolveSession = ResolveSession,
})
