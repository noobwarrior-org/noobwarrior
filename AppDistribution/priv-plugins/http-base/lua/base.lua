-- ////////////////////////////////////////////////////////////////////////////////
-- noobWarrior
-- Plugin: HTTP Server Base
-- File: base.lua
-- Description: Main file for HTTP Server Base
-- Started by: Hattozo
-- Started on: 1/3/2026
-- ////////////////////////////////////////////////////////////////////////////////
print("hello from base.lua")
_G.HTTP_BASE_VER = "0.1"
local http_base = {}

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

function http_base.GetFileExtension(filePath)
    local pos = string.reverse(filePath):find("%.")
    return string.sub(filePath, 1 - pos)
end

function http_base.ReadFileBinary(vfs, localUrl)
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

function http_base.AttachToServer(srv, params)
    srv.OnRequest:Connect(function(req)
        local get_tbl = {}
        local post_tbl = {}
        local cookies_tbl = {}

        local uri_query_pos = string.find(req.Uri, "?")
        local uri_without_params = uri_query_pos and string.sub(req.Uri, 1, uri_query_pos - 1) or req.Uri
        local uri_with_params_only = uri_query_pos and string.sub(req.Uri, uri_query_pos + 1) or ""
        for param in string.gmatch(uri_with_params_only, '([^&]+)') do
            local equals_index = string.find(param, "=")
            if equals_index then
                local key = string.sub(param, 1, equals_index - 1)
                local val = string.sub(param, equals_index + 1, -1)
                get_tbl[key] = val
            end
        end

        if req.PostBody ~= nil then
            for param in string.gmatch(req.PostBody, '([^&]+)') do
                local equals_index = string.find(param, "=")
                if equals_index then
                    local key = string.sub(param, 1, equals_index - 1)
                    local val = string.sub(param, equals_index + 1, -1)
                    post_tbl[key] = val
                end
            end
        end

        if req.Headers and req.Headers["Cookie"] then
            for key, value in req.Headers["Cookie"]:gmatch("([^%s=]+)=([^;]+)") do
                cookies_tbl[key] = value
            end
        end

        if params.Sitemap[uri_without_params] then
            req:AddHeader("Content-Type", "text/html")
            local success, err = pcall(function()
                local output = lhp.RenderFile(params.Sitemap[uri_without_params], {
                    ["_SERVER"] = {},
                    ["_GET"] = get_tbl,
                    ["_POST"] = post_tbl,
                    ["_FILES"] = {},
                    ["_COOKIE"] = cookies_tbl,
                    ["_SESSION"] = {},
                    ["_REQUEST"] = {},
                    ["_ENV"] = {},
                    ["header"] = function(header, replace, response_code)
                        local key, value = string.match(header, "([^:]+):%s*(.*)")
                        req:AddHeader(key, value)
                    end,
                    ["setcookie"] = function(name, value, expires, path, domain, secure, httponly)
                        assert(name ~= nil, "Parameter #1 \"name\" cannot be nil")
                        
                        expires = expires or 0
                        path = path or ""
                        domain = domain or ""
                        if secure == nil then
                            secure = false
                        end
                        if httponly == nil then
                            httponly = false
                        end

                        -- deletes the cookie if value is empty
                        if value == nil or string.match(value, "^%s*$") then
                            value = "deleted"
                            expires = 1
                        end

                        local cookieHeader = string.format("%s=%s; Max-Age=%d; Path=%s; Domain=%s", name, tostring(value), expires - os.time(), path, domain)
                        if secure then
                            cookieHeader = cookieHeader .. "; Secure"
                        end
                        if httponly then
                            cookieHeader = cookieHeader .. "; HttpOnly"
                        end
                        req:AddHeader("Set-Cookie", cookieHeader)
                    end
                })
                req:SendReply(200, nil, output)
            end)
            if not success then
                req:SendError(500, "LHP Error: Failed to render page \""..params.Sitemap[uri_without_params].."\"<br>"..err)
            end
        else
            local vfs = srv:GetVfs()
            if vfs:EntryExists(uri_without_params) then
                local data = http_base.ReadFileBinary(vfs, uri_without_params)
                if data == nil then
                    error("Failed to read binary data from "..uri_without_params)
                end
                local mimeType = file_extension_map[http_base.GetFileExtension(uri_without_params)]
                req:AddHeader("Content-Type", mimeType == nil and "application/octet-stream" or mimeType)
                req:SendReply(200, nil, data)
            else
                print("File", uri_without_params, "doesn't exist!")
                req:AddHeader("Content-Type", "text/html")
                req:SendError(404, "This page was not found!")
            end
        end
    end)
    srv:MountVolume("/", "/static")
end

function http_base.CreateServer(params)
    local srv = HttpServer.new(params.Name)
    http_base.AttachToServer(srv, params)
    return srv
end

return http_base