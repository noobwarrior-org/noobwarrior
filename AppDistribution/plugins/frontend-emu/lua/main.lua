-- ////////////////////////////////////////////////////////////////////////////////
-- noobWarrior
-- Plugin: Server Emulator Frontend
-- File: main.lua
-- Description: Main entrypoint for the server emulator frontend
-- Started by: Hattozo
-- Started on: 1/3/2026
-- ////////////////////////////////////////////////////////////////////////////////
emu.OnRequest:Connect(function(request)

end)

emu.Request:Connect(function(request)
    plugin.GetDirectory()
end)
