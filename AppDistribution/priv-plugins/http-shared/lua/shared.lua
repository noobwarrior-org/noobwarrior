-- ////////////////////////////////////////////////////////////////////////////////
-- noobWarrior
-- Plugin: HTTP Server Shared
-- File: shared.lua
-- Description: Main file for HTTP Server Shared
-- Started by: Hattozo
-- Started on: 1/3/2026
-- ////////////////////////////////////////////////////////////////////////////////
print("hello from shared.lua")
local http_shared = {}

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

function http_shared.GetFileExtension(filePath)
    local pos = string.reverse(filePath):find("%.")
    return string.sub(filePath, 1 - pos)
end

function http_shared.ReadFileBinary(vfs, localUrl)
    if not vfs:EntryExists(localUrl) then
        return nil
    end
    local handle = vfs:OpenHandle(localUrl)
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

    return fullData
end

function http_shared.AttachToServer(srv, params)
    srv.OnRequest:Connect(function(req)
        if params.Sitemap[req.Uri] then
            req:AddHeader("Content-Type", "text/html")
            local success, err = pcall(function()
                local output = lhp.RenderFile(params.Sitemap[req.Uri])
                req:SendReply(200, nil, output)
            end)
            if not success then
                req:SendError(500, "LHP Error: Failed to render page \""..params.Sitemap[req.Uri].."\"")
            end
        else
            local vfs = srv:GetVfs()
            if vfs:EntryExists(req.Uri) then
                local data = http_shared.ReadFileBinary(vfs, req.Uri)
                if data == nil then
                    error("Failed to read binary data from "..req.Uri)
                end
                local mimeType = file_extension_map[http_shared.GetFileExtension(req.Uri)]
                req:AddHeader("Content-Type", mimeType == nil and "application/octet-stream" or mimeType)
                req:SendReply(200, nil, data)
            else
                print("File", req.Uri, "doesn't exist!")
                req:AddHeader("Content-Type", "text/html")
                req:SendError(404, "This page was not found!")
            end
        end
    end)
    srv:MountVolume("/", "/static")
end

function http_shared.CreateServer(params)
    local srv = HttpServer.new(params.Name)
    http_shared.AttachToServer(srv, params)
    return srv
end

return http_shared