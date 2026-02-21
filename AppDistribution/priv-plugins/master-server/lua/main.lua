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

print(lhp)
print(script)
local res = lhp.RenderFile("/src/index.lhp")

master.OnRequest:Connect(function(req)
    print("Request from "..req.Ip.." made")
    local res = lhp.RenderFile("/src/index.lhp")
end)
