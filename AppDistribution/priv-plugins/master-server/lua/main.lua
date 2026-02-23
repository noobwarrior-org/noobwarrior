-- ////////////////////////////////////////////////////////////////////////////////
-- noobWarrior
-- Plugin: Master Server
-- File: main.lua
-- Description: Main entrypoint for Master Server plugin
-- Started by: Hattozo
-- Started on: 1/3/2026
-- ////////////////////////////////////////////////////////////////////////////////
master = HttpServer.new("MasterServer")

master_db = SqlDb.new(":memory:", "MasterDb")

local success, msg = pcall(function()
    local res = lhp.RenderFile("/src/index.lhp")
end)

master.OnRequest:Connect(function(req)
    print("Request from "..req.PeerIp.." made to "..req.Uri)
    local res = lhp.RenderFile("/src/index.lhp")
    if req.Uri == "/" or req.Uri == "/home" then
        req:AddHeader("Content-Type", "text/html")
        req:SendReply(200, nil, "YOU ARE ON THE HOMEPAGE")
    else
        req:AddHeader("Content-Type", "text/html")
        req:SendError(404, "This page was not found!")
    end
end)

master:Start(4040)
