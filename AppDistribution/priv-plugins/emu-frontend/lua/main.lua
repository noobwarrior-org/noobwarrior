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
    ["/library/:id"] = "/src/asset_preview.lhp",
    ["/library/:id/"] = "/src/asset_preview.lhp",
    ["/library/:id/:name"] = "/src/asset_preview.lhp",
    
    ["/v1/settings/branding"] = { Page = "/src/api/settings/branding.lhp", Permission = "access.control_panel" },

    ["/v1/ranks/save"] = { Page = "/src/api/ranks/save.lhp", Permission = "control_panel.ranks" },
    ["/v1/ranks/delete"] = { Page = "/src/api/ranks/delete.lhp", Permission = "control_panel.ranks" },
    ["/v1/ranks/set-permission"] = { Page = "/src/api/ranks/set_permission.lhp", Permission = "control_panel.ranks" },

    ["/v1/items/upload"] = { Page = "/src/api/items/upload.lhp", Permission = "items.write" },
    ["/v1/items/icon"] = { Page = "/src/api/items/icon.lhp", Permission = "items.write" },

    ["/v1/forums/create-category"] = { Page = "/src/api/forums/create_category.lhp", Permission = "forums.structure" },
    ["/v1/forums/update-category"] = { Page = "/src/api/forums/update_category.lhp", Permission = "forums.structure" },
    ["/v1/forums/delete-category"] = { Page = "/src/api/forums/delete_category.lhp", Permission = "forums.structure" },
    ["/v1/forums/create-forum"] = { Page = "/src/api/forums/create_forum.lhp", Permission = "forums.structure" },
    ["/v1/forums/update-forum"] = { Page = "/src/api/forums/update_forum.lhp", Permission = "forums.structure" },
    ["/v1/forums/delete-forum"] = { Page = "/src/api/forums/delete_forum.lhp", Permission = "forums.structure" },
    ["/v1/forums/create-thread"] = { Page = "/src/api/forums/create_thread.lhp", Permission = "forums.post" },
    ["/v1/forums/reply"] = { Page = "/src/api/forums/reply.lhp", Permission = "forums.post" },
    ["/v1/forums/edit-post"] = { Page = "/src/api/forums/edit_post.lhp", Permission = "forums.post" },
    ["/v1/forums/delete-post"] = { Page = "/src/api/forums/delete_post.lhp", Permission = "forums.post" }
}

http_base.AttachToServer(emu, {
    Sitemap = sitemap,
})
