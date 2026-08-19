-- ////////////////////////////////////////////////////////////////////////////////
-- noobWarrior
-- Plugin: Overlay
-- File: init.server.lua
-- Description: Main file for HTTP Server Base
-- Started by: Hattozo
-- Started on: 8/16/2026
-- ////////////////////////////////////////////////////////////////////////////////
local ReplicatedStorage = game:GetService("ReplicatedStorage")
local HttpService = game:GetService("HttpService")

local RNoob = Instance.new("Folder")
RNoob.Name = "NoobWarriorOverlay"
RNoob.Parent = ReplicatedStorage

local function RegisterFunc(name, func)
    local r = Instance.new("RemoteFunction")
    r.Name = name
    r.OnServerInvoke = func
    r.Parent = RNoob
    return r
end

local SendCommand = RegisterFunc("SendCommand", function(player: Player, cmd)
    -- HttpService blocks any requests to roblox.com so that they dont DoS their own servers.
    -- noobHook redirects every URL call to our server emulator regardless if it has roblox.com in the name, so we're just doing that for now.
    HttpService:PostAsync("http://noobwarrior.org/emu/v1/")
    return nil
end)
