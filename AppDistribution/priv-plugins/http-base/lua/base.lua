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

local SESSION_COOKIE = "NWSESSID"
local SESSION_STORE = {} -- { [id] = { data = {}, last_used = number } }
local _SESSION_ID_COUNTER = 0
math.randomseed(os.time())

local function generate_session_id()
    _SESSION_ID_COUNTER = _SESSION_ID_COUNTER + 1
    local chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
    local id = ""
    for i = 1, 40 do
        local r = math.random(1, #chars)
        id = id .. string.sub(chars, r, r)
    end
    return id .. tostring(_SESSION_ID_COUNTER)
end

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

local function match_sitemap(sitemap, uri)
    if sitemap[uri] then
        return sitemap[uri], {}
    end
    for pattern, entry in pairs(sitemap) do
        if pattern:find(":[%a_]") then
            local names = {}
            -- WTF
            local lua_pattern = "^"
                .. pattern
                    :gsub("([%.%+%-%*%?%[%]%^%$%(%)%%])", "%%%1")
                    :gsub(":([%a_][%w_]*)", function(name)
                        names[#names + 1] = name
                        return "([^/]+)"
                    end)
                .. "$"
            local captures = { uri:match(lua_pattern) }
            if #captures > 0 then
                local params = {}
                for i, val in ipairs(captures) do
                    params[names[i]] = val
                end
                return entry, params
            end
        end
    end
    return nil, {}
end

function http_base.AttachToServer(srv, params)
    if params.Sitemap then
        for uri, entry in pairs(params.Sitemap) do
            params.Sitemap[uri] = url.ResolveFromCaller(entry)
        end
    end
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

        local sitemap_entry, url_params = match_sitemap(params.Sitemap, uri_without_params)
        if sitemap_entry then
            local session_tbl = {}
            local session_id = nil
            local session_started = false

            local function do_session_start()
                if session_started then return end
                session_started = true
                local sid = cookies_tbl[SESSION_COOKIE]
                if sid and SESSION_STORE[sid] then
                    session_id = sid
                    SESSION_STORE[sid].last_used = os.time()
                    for k, v in pairs(SESSION_STORE[sid].data) do
                        rawset(session_tbl, k, v)
                    end
                else
                    session_id = generate_session_id()
                    SESSION_STORE[session_id] = { data = {}, last_used = os.time() }
                    req:AddHeader("Set-Cookie", string.format("%s=%s; Path=/; HttpOnly", SESSION_COOKIE, session_id))
                end
            end

            local function session_destroy()
                if session_id then
                    SESSION_STORE[session_id] = nil
                    req:AddHeader("Set-Cookie", string.format("%s=deleted; Max-Age=0; Path=/; HttpOnly", SESSION_COOKIE))
                    session_id = nil
                    for k in pairs(session_tbl) do session_tbl[k] = nil end
                end
            end

            local session_proxy = setmetatable({}, {
                __index    = function(_, k) do_session_start(); return session_tbl[k] end,
                __newindex = function(_, k, v) do_session_start(); session_tbl[k] = v end,
                __pairs    = function(_) do_session_start(); return pairs(session_tbl) end,
            })

            req:AddHeader("Content-Type", "text/html")
            local response_code = 200

            local success, result = pcall(function()
                return lhp.RenderFile(sitemap_entry, {
                    ["_SERVER"] = {
                        LHP_SELF = uri_without_params,
                        SCRIPT_FILENAME = sitemap_entry,
                        SERVER_NAME = req.Headers["Host"] or "",
                        HTTP_HOST = req.Headers["Host"] or "",
                        HTTP_USER_AGENT = req.Headers["User-Agent"] or "",
                        REQUEST_METHOD = req.Method or "GET",
                        REMOTE_ADDR = req.PeerIp or "",
                        QUERY_STRING = uri_with_params_only,
                    },
                    ["_GET"] = get_tbl,
                    ["_POST"] = post_tbl,
                    ["_RAW_POST"] = req.PostBody or "",
                    ["_FILES"] = {},
                    ["_COOKIE"] = cookies_tbl,
                    ["_PARAMS"] = url_params,
                    ["_SESSION"] = session_proxy,
                    ["_REQUEST"] = {},
                    ["_ENV"] = {},
                    ["session_destroy"] = session_destroy,
                    ["header"] = function(h, replace, rc)
                        if replace == nil then
                            replace = true
                        end
                        local status = string.match(h, "^HTTP/%S+ (%d+)")
                        if status then
                            response_code = tonumber(status)
                        else
                            local key, value = string.match(h, "([^:]+):%s*(.*)")
                            if key and value then
                                if replace then req:RemoveHeader(key) end
                                req:AddHeader(key, value)
                            end
                        end
                        if rc ~= nil then response_code = rc end
                    end,
                    ["http_response_code"] = function(code)
                        if code ~= nil then response_code = code end
                        return response_code
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
            end)

            -- persist any _SESSION mutations back to the store
            if session_id and SESSION_STORE[session_id] then
                SESSION_STORE[session_id].data = {}
                for k, v in pairs(session_tbl) do
                    SESSION_STORE[session_id].data[k] = v
                end
            end

            if success then
                req:SendReply(response_code, nil, result)
            else
                req:SendError(500, "LHP Error: Failed to render page \""..sitemap_entry.."\"<br>"..result)
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