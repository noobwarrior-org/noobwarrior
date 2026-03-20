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
    if req.Uri == "/" or req.Uri == "/home" then
        local output = lhp.RenderFile("/src/index.lhp")
        print(output)

        req:AddHeader("Content-Type", "text/html")
        req:SendReply(200, nil, output)
    else
        local vfs = master:GetVfs()
        local entries = vfs:GetEntriesInDirectory("/")
        for index, entry in ipairs(entries) do
            print(string.format("Entry %s: %s", index, entry.Name))

        end
        if vfs:EntryExists(req.Uri) then
            local handle = vfs:OpenHandle(req.Uri)
            print("Vfs:", vfs)
            print("Handle:", handle)
            if handle == 0 then
                error("Failed to open handle!")
            end

            print("Opened handle! Reading...")
            local fullData = ""
            local isReading, chunkData = vfs:ReadHandleChunk(handle, 1024)
            if not isReading then
                error("Failed to read chunk from handle!")
            end
            while isReading do
                print(isReading, chunkData)
                if isReading then
                    fullData = fullData..chunkData
                end
                isReading, chunkData = vfs:ReadHandleChunk(handle, 1024)
            end

            vfs:CloseHandle(handle)

            req:AddHeader("Content-Type", "application/octet-stream")
            req:AddHeader("Content-Disposition", "inline")
            req:SendReply(200, nil, fullData)
        else
            print("File", req.Uri, "doesn't exist!")
            req:AddHeader("Content-Type", "text/html")
            req:SendError(404, "This page was not found!")
        end
    end
end)
master:MountVolume("/", "plugin://frontend@noobwarrior.org/static")
-- master:UnmountVolume("/", "plugin://frontend@noobwarrior.org/static")
master:Start(4040)
