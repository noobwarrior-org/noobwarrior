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

local sitemap = {
    ["/"] = "index.lhp",
    ["/home"] = "index.lhp"
}

local file_extension_map = {
    ["txt"] = "text/plain",
    ["css"] = "text/css",
    ["csv"] = "text/csv",
    ["html"] = "text/html",
    ["xml"] = "text/xml",
    ["png"] = "image/png",
    ["jpg"] = "image/jpeg",
    ["jpeg"] = "image/jpeg",
    ["tiff"] = "image/tiff",
    ["ico"] = "image/vnd.microsoft.icon",
    ["svg"] = "image/svg+xml",
    ["mp4"] = "video/mp4",
    ["webm"] = "video/webm"
}

local function getFileExtension(filePath)
    local pos = string.reverse(filePath):find("%.")
    return string.sub(filePath, 1 - pos)
end

master.OnRequest:Connect(function(req)
    print("Request from "..req.PeerIp.." made to "..req.Uri)
    if sitemap[req.Uri] then
        local output = lhp.RenderFile("/src/"..sitemap[req.Uri])
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
            if handle == 0 then
                error("Failed to open handle!")
            end

            local fullData = ""
            local isReading, chunkData = true, nil
            repeat
                isReading, chunkData = vfs:ReadHandleChunk(handle, 4096)
                if chunkData and chunkData ~= "" then
                    fullData = fullData .. chunkData
                end
            until not isReading

            vfs:CloseHandle(handle)

            local mimeType = file_extension_map[getFileExtension(req.Uri)]
            req:AddHeader("Content-Type", mimeType == nil and "application/octet-stream" or mimeType)
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
