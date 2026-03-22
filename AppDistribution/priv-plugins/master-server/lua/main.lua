-- ////////////////////////////////////////////////////////////////////////////////
-- noobWarrior
-- Plugin: Master Server
-- File: main.lua
-- Description: Main entrypoint for Master Server plugin
-- Started by: Hattozo
-- Started on: 1/3/2026
-- ////////////////////////////////////////////////////////////////////////////////
master_db = SqlDb.new(":memory:", "MasterDb")

local http_shared = require("plugin://http-shared@noobwarrior.org/lua/shared.lua")

local sitemap = {
    ["/"] = "plugin://master-server@noobwarrior.org/src/index.lhp",
    ["/home"] = "plugin://master-server@noobwarrior.org/src/index.lhp"
}

master = http_shared.CreateServer({
    Name = "MasterServer",
    Sitemap = sitemap
})
master:MountVolume("/", "/static") -- also mount our own static directory alongside the shared http server's
master:Start(4040)
