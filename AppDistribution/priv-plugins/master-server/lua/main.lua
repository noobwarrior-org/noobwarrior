-- ////////////////////////////////////////////////////////////////////////////////
-- noobWarrior
-- Plugin: Master Server
-- File: main.lua
-- Description: Main entrypoint for Master Server plugin
-- Started by: Hattozo
-- Started on: 1/3/2026
-- ////////////////////////////////////////////////////////////////////////////////
master = HttpServer.new("MasterServer")
master:Start(4040)

master_db = SqlDb.new(":memory:", "MasterDb")

local success, msg = pcall(function()
    local res = lhp.RenderFile("/src/index.lhp")
end)

master.OnRequest:Connect(function(req)
    print("Request from "..req.PeerIp.." made")
    local res = lhp.RenderFile("/src/index.lhp")
    req:AddHeader("Content-Type", "text/html")
    req:SendReply("what's up gamers", 200, nil)
end)
