-- ////////////////////////////////////////////////////////////////////////////////
-- noobWarrior
-- Plugin: Documentation
-- File: main.lua
-- Description: Main entrypoint for the documentation plugin
-- Started by: Hattozo
-- Started on: 5/30/2026
-- ////////////////////////////////////////////////////////////////////////////////
reg.SetKeyValueIfNotSet("docs.autostart", false)
reg.SetKeyComment("docs.autostart", "Set to true to automatically make the documentation webserver start listening on startup.")

reg.SetKeyValueIfNotSet("docs.http_port", 5000)
reg.SetKeyComment("docs.http_port", "Port the documentation HTTP webserver binds to.")

reg.SetKeyValueIfNotSet("docs.https_port", 5050)
reg.SetKeyComment("docs.https_port", "Port the documentation HTTPS webserver binds to.")

_G.DOCS_VER = "0.1"
local http_base = require("plugin://http-base@noobwarrior.org/lua/base.lua")

local sitemap = {
    ["/"] = "/src/index.lhp"
}

docs = http_base.CreateServer({
    Name = "Documentation",
    Sitemap = sitemap
})
docs:MountVolume("/", "/static") -- also mount our own static directory alongside the shared http server's
if reg.GetKeyValue("docs.autostart") then
    local http_port = reg.GetKeyValue("docs.http_port") or 5000
    local https_port = reg.GetKeyValue("docs.https_port") or 5050
    docs:Start(http_port)
    docs:StartSecure(https_port)
    print(string.format("Automatically started documentation webserver! HTTP server listening on port %d and HTTPS server listening on port %d", http_port, https_port))
end
