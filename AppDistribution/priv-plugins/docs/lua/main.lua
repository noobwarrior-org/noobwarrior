-- ////////////////////////////////////////////////////////////////////////////////
-- noobWarrior
-- Plugin: Documentation
-- File: main.lua
-- Description: Main entrypoint for the documentation plugin
-- Started by: Hattozo
-- Started on: 5/30/2026
-- ////////////////////////////////////////////////////////////////////////////////
_G.DOCS_VER = "0.1"
local http_base = require("plugin://http-base@noobwarrior.org/lua/base.lua")

local sitemap = {
    ["/"] = "plugin://docs@noobwarrior.org/src/index.lhp"
}