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

local http_shared = require("plugin://http-shared@noobwarrior.org/lua/shared.lua")

local sitemap = {
    ["/"] = "plugin://emu-frontend@noobwarrior.org/src/index.lhp",
    ["/login"] = "plugin://emu-frontend@noobwarrior.org/src/login.lhp",
    ["/register"] = "plugin://emu-frontend@noobwarrior.org/src/register.lhp",
    ["/servers"] = "plugin://emu-frontend@noobwarrior.org/src/servers.lhp",
    ["/control-panel"] = "plugin://emu-frontend@noobwarrior.org/src/controlpanel.lhp"
}

http_shared.AttachToServer(emu, {
    Sitemap = sitemap
})
