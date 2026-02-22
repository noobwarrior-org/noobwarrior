-- ////////////////////////////////////////////////////////////////////////////////
-- noobWarrior
-- Plugin: Master Server
-- File: main.lua
-- Description: Main entrypoint for Master Server plugin
-- Started by: Hattozo
-- Started on: 1/3/2026
-- ////////////////////////////////////////////////////////////////////////////////
master = HttpServer.new()
master:Start(4040)

local signal = Signal.new()
local listener1 = signal:Connect(function()
    print("hello")
end)
local listener2 = signal:Connect(function()
    print("HAHA LOL!")
end)
local listener3 = signal:Connect(function(str)
    print("What is up", str)
end)
signal:Fire("testy testers")

local db = SqlDb.new(":memory:", "MasterDb")

print(lhp)
print(script)

local success, msg = pcall(function()
    local res = lhp.RenderFile("/src/index.lhp")
end)

master.OnRequest:Connect(function(req)
    print("Request from "..req.PeerIp.." made")
    local res = lhp.RenderFile("/src/index.lhp")
end)
