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

master.OnRequest:Connect(function(req)
    print("Request from "..req.PeerIp.." made to "..req.Uri)
    local output = lhp.RenderFile("/src/index.lhp")
    print(output)
    if req.Uri == "/" or req.Uri == "/home" then
        req:AddHeader("Content-Type", "text/html")
        req:SendReply(200, nil, output)
    else
        req:AddHeader("Content-Type", "text/html")
        req:SendError(404, "This page was not found!")
    end
end)
master:MountVolume("/", "plugin://frontend@noobwarrior.org/static")
-- master:UnmountVolume("/", "plugin://frontend@noobwarrior.org/static")
master:Start(4040)
