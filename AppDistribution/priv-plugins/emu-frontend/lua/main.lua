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
    ["/"] = "/src/index.lhp",
    ["/login"] = "/src/login.lhp",
    ["/register"] = "/src/register.lhp",
    ["/servers"] = "/src/servers.lhp",
    ["/forums"] = "/src/forums.lhp",
    ["/control-panel"] = "/src/controlpanel.lhp",
    ["/users/:userId/profile"] = "/src/profile.lhp"
}

http_base.AttachToServer(emu, {
    Sitemap = sitemap,
})
