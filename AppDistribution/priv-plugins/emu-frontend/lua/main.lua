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
    ["/develop"] = "/src/develop.lhp",
    ["/develop/:category"] = "/src/develop.lhp",
    ["/forums"] = "/src/forums.lhp",
    ["/forums/new-thread"] = "/src/forums_newthread.lhp",
    ["/forums/edit-post"] = "/src/forums_editpost.lhp",
    ["/forums/search"] = "/src/forums_search.lhp",
    ["/control-panel"] = "/src/controlpanel.lhp",
    ["/users/:userId/profile"] = "/src/profile.lhp",

    ["/v1/forums/create-category"] = "/src/api/forums/create_category.lhp",
    ["/v1/forums/update-category"] = "/src/api/forums/update_category.lhp",
    ["/v1/forums/delete-category"] = "/src/api/forums/delete_category.lhp",
    ["/v1/forums/create-forum"] = "/src/api/forums/create_forum.lhp",
    ["/v1/forums/update-forum"] = "/src/api/forums/update_forum.lhp",
    ["/v1/forums/delete-forum"] = "/src/api/forums/delete_forum.lhp",
    ["/v1/forums/create-thread"] = "/src/api/forums/create_thread.lhp",
    ["/v1/forums/reply"] = "/src/api/forums/reply.lhp",
    ["/v1/forums/edit-post"] = "/src/api/forums/edit_post.lhp",
    ["/v1/forums/delete-post"] = "/src/api/forums/delete_post.lhp"
}

http_base.AttachToServer(emu, {
    Sitemap = sitemap,
})
