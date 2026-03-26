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
emu:MountVolume("/", "/static")

local http_shared = require("plugin://http-shared@noobwarrior.org/lua/shared.lua")

local sitemap = {
    ["/"] = "plugin://emu-frontend@noobwarrior.org/src/index.lhp"
}

http_shared.AttachToServer(emu, {
    Sitemap = sitemap
})
