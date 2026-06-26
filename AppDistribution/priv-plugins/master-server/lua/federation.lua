-- ////////////////////////////////////////////////////////////////////////////////
-- noobWarrior
-- Plugin: Master Server
-- File: federation.lua
-- Description:
-- Started by: Hattozo
-- Started on: 6/25/2026
-- ////////////////////////////////////////////////////////////////////////////////
local fed = {}

local function db() return _G.MASTERSERVER_DB end

local function firstRow(result)
    return type(result) == "table" and result[1] or nil
end

local function asList(result)
    return type(result) == "table" and result or {}
end

local function blank(s)
    return s == nil or tostring(s):match("^%s*$") ~= nil
end

local function rstrip(url)
    return (tostring(url or ""):gsub("/+$", ""))
end

local function newClient()
    local c = NetClient.new()
    c:SetTimeout(8)
    return c
end

local function httpGet(url)
    local res = newClient():Get(url)
    if not res or not res.Ok then return nil, (res and res.Error) or "request failed" end
    return res
end

local function httpPost(url, payload)
    local res = newClient():Post(url, json.stringify(payload), "application/json")
    if not res or not res.Ok then return nil, (res and res.Error) or "request failed" end
    return res
end

local function parseJson(body)
    if type(body) ~= "string" then return nil end
    local ok, v = pcall(json.parse, body)
    return (ok and type(v) == "table") and v or nil
end

function fed.ParseIdentity(identifier)
    identifier = tostring(identifier or "")
    local at = identifier:find("@[^@]*$")
    if not at then return nil end
    local username, domain = identifier:sub(1, at - 1), identifier:sub(at + 1)
    if username == "" or domain == "" then return nil end
    return username, domain
end

function fed.IsLocalDomain(domain)
    return tostring(domain):lower() == _G.MASTERSERVER_DOMAIN():lower()
end

function fed.SelfBaseUrl()
    local url = reg.GetKeyValue("master.public_url")
    if not blank(url) then return rstrip(url) end
    return "https://" .. _G.MASTERSERVER_DOMAIN()
end

function fed.AutoEnabled()
    local v = reg.GetKeyValue("master.federation.auto")
    return v ~= false and v ~= "false" and v ~= 0
end

-- Peers

function fed.GetPeers()
    local rows = db():QueryTyped(
        "SELECT Id, Domain, BaseUrl, Name, FirstSeen, LastSeen, Status FROM Peer ORDER BY Domain ASC;")
    return asList(rows)
end

function fed.ResolveBaseUrl(domain)
    local row = firstRow(db():QueryTyped("SELECT BaseUrl FROM Peer WHERE Domain = ? COLLATE NOCASE;", domain))
    if row and row.BaseUrl then return rstrip(row.BaseUrl) end
    return "https://" .. tostring(domain)
end

local function peerKnown(domain)
    return firstRow(db():QueryTyped("SELECT 1 FROM Peer WHERE Domain = ? COLLATE NOCASE;", domain)) ~= nil
end

function fed.AddPeer(baseUrl)
    baseUrl = rstrip(baseUrl)
    if baseUrl == "" then return nil, "empty url" end

    local res, err = httpGet(baseUrl .. "/fed/v1/info")
    if not res then return nil, "could not reach peer: " .. tostring(err) end
    local info = parseJson(res.Body)
    if not info or not info.Domain then return nil, "peer did not return valid federation info" end

    local domain = tostring(info.Domain)
    if fed.IsLocalDomain(domain) then return nil, "that peer is this server" end
    local name = tostring(info.Name or domain)

    if peerKnown(domain) then
        db():QueryTyped(
            "UPDATE Peer SET BaseUrl = ?, Name = ?, LastSeen = unixepoch(), Status = 'active' WHERE Domain = ? COLLATE NOCASE;",
            baseUrl, name, domain)
    else
        db():QueryTyped("INSERT INTO Peer (Domain, BaseUrl, Name) VALUES (?, ?, ?);", domain, baseUrl, name)
    end
    return { Domain = domain, BaseUrl = baseUrl, Name = name }
end

function fed.AutoAddPeer(domain, baseUrl)
    if domain == nil or fed.IsLocalDomain(domain) or peerKnown(domain) then return end
    baseUrl = blank(baseUrl) and ("https://" .. domain) or rstrip(baseUrl)
    db():QueryTyped("INSERT INTO Peer (Domain, BaseUrl, Name) VALUES (?, ?, ?);", domain, baseUrl, domain)
end

function fed.AutoAccept(fromIdentity, baseUrl)
    if not fed.AutoEnabled() then return end
    local _, domain = fed.ParseIdentity(fromIdentity)
    fed.AutoAddPeer(domain, baseUrl)
end

function fed.GossipPeers()
    if not fed.AutoEnabled() then return end
    for _, peer in ipairs(fed.GetPeers()) do
        local res = httpGet(rstrip(peer.BaseUrl) .. "/fed/v1/peers")
        local data = res and parseJson(res.Body)
        if data and type(data.Peers) == "table" then
            for _, p in ipairs(data.Peers) do fed.AutoAddPeer(p.Domain, p.BaseUrl) end
        end
    end
end

-- Origin-callback verification

local function deliver(peerDomain, path, payload)
    payload.OriginBaseUrl = fed.SelfBaseUrl()
    local res, err = httpPost(fed.ResolveBaseUrl(peerDomain) .. path, payload)
    if not res then return false, "could not deliver to " .. peerDomain .. ": " .. tostring(err) end
    if res.Status and res.Status >= 400 then
        local body = parseJson(res.Body)
        return false, (body and body.Error) or ("remote rejected (HTTP " .. tostring(res.Status) .. ")")
    end
    return true
end

local function recordOutbound(actionId, fromUserId, fromUsername, toIdentity, body)
    return db():QueryTyped(
        "INSERT INTO OutboundMessage (ActionId, FromUserId, FromUsername, ToIdentity, Body, BodyHash) VALUES (?, ?, ?, ?, ?, ?);",
        actionId, fromUserId, fromUsername, toIdentity, body, hash.Sha256(body)) ~= false
end

function fed.LookupOutbound(actionId)
    return firstRow(db():QueryTyped(
        "SELECT FromUsername, ToIdentity, BodyHash, CreatedTimestamp FROM OutboundMessage WHERE ActionId = ?;", actionId))
end

function fed.VerifyRemoteAction(fromIdentity, actionId, body)
    local _, domain = fed.ParseIdentity(fromIdentity)
    if not domain then return false, "malformed sender identity" end

    local res, err = httpGet(fed.ResolveBaseUrl(domain) .. "/fed/v1/verify?action=" .. _G.MASTERSERVER_URL_ENCODE(actionId))
    if not res then return false, "origin unreachable: " .. tostring(err) end
    local vouch = parseJson(res.Body)
    if not vouch or not vouch.Ok then return false, "origin did not vouch for this action" end

    -- A domain may only vouch for its own users, and only for who the action claimed.
    local _, actorDomain = fed.ParseIdentity(vouch.Actor or "")
    if not actorDomain or actorDomain:lower() ~= domain:lower() then
        return false, "origin vouched for a foreign identity"
    end
    if tostring(vouch.Actor):lower() ~= fromIdentity:lower() then return false, "actor mismatch" end
    if tostring(vouch.BodyHash or "") ~= hash.Sha256(tostring(body)) then return false, "body hash mismatch" end
    return true
end

function fed.ActionSeen(actionId)
    return firstRow(db():QueryTyped("SELECT 1 FROM ReceivedAction WHERE ActionId = ? LIMIT 1;", actionId)) ~= nil
end

function fed.MarkActionSeen(actionId, kind)
    db():QueryTyped("INSERT OR IGNORE INTO ReceivedAction (ActionId, Kind) VALUES (?, ?);", actionId, kind or "")
end

-- Identity

function fed.LocalUserProfile(username)
    local m = core.GetMasterDatabase()
    if m == nil then return nil, "no master database" end
    local row = firstRow(m:QueryTyped("SELECT Id, Name, DisplayName FROM User WHERE Name = ? COLLATE NOCASE;", username))
    if not row then return nil, "user not found" end
    local fullId = _G.MASTERSERVER_FULL_IDENTITY(row.Name)
    return {
        Identity    = fullId,
        UserId      = _G.MASTERSERVER_ONLINE_USER_ID(fullId),
        Name        = row.Name,
        DisplayName = row.DisplayName or row.Name,
        Domain      = _G.MASTERSERVER_DOMAIN(),
    }
end

function fed.ResolveUser(identifier)
    local username, domain = fed.ParseIdentity(identifier)
    if not username then username, domain = tostring(identifier), _G.MASTERSERVER_DOMAIN() end
    if fed.IsLocalDomain(domain) then return fed.LocalUserProfile(username) end

    local res, err = httpGet(fed.ResolveBaseUrl(domain) .. "/fed/v1/users/" .. _G.MASTERSERVER_URL_ENCODE(username))
    if not res then return nil, err end
    if res.Status and res.Status >= 400 then return nil, "remote user not found" end
    local profile = parseJson(res.Body)
    if not profile then return nil, "invalid profile response" end
    return profile
end

local function localUserRow(username)
    local m = core.GetMasterDatabase()
    return m and firstRow(m:QueryTyped("SELECT Id, Name FROM User WHERE Name = ? COLLATE NOCASE;", username))
end

-- Direct messages

function fed.SendMessage(fromUserId, fromUsername, toIdentity, body)
    local toUser, toDomain = fed.ParseIdentity(toIdentity)
    if not toUser then toUser, toDomain = tostring(toIdentity), _G.MASTERSERVER_DOMAIN() end
    if blank(body) then return false, "message body is empty" end
    body = tostring(body)
    local fromIdentity = _G.MASTERSERVER_FULL_IDENTITY(fromUsername)

    if fed.IsLocalDomain(toDomain) then
        local recipient = localUserRow(toUser)
        if not recipient then return false, "no such user on this server" end
        db():QueryTyped(
            "INSERT INTO Message (FromIdentity, ToUserId, ToUsername, Body, Verified) VALUES (?, ?, ?, ?, 1);",
            fromIdentity, recipient.Id, recipient.Name, body)
        return true
    end

    local actionId = hash.GenerateToken()
    if not recordOutbound(actionId, fromUserId, fromUsername, _G.MASTERSERVER_FULL_IDENTITY(toUser, toDomain), body) then
        return false, "failed to record outbound message"
    end
    return deliver(toDomain, "/fed/v1/inbox", { From = fromIdentity, To = toUser, Body = body, ActionId = actionId })
end

function fed.ReceiveMessage(from, toUsername, body, actionId)
    if not fed.ParseIdentity(from) then return false, "malformed sender identity" end
    if blank(actionId) then return false, "missing action id" end
    if blank(toUsername) then return false, "missing recipient" end
    body = tostring(body or "")

    local recipient = localUserRow(toUsername)
    if not recipient then return false, "no such user on this server" end
    if firstRow(db():QueryTyped("SELECT 1 FROM Message WHERE ActionId = ? LIMIT 1;", actionId)) then
        return false, "duplicate message"
    end

    local ok, err = fed.VerifyRemoteAction(from, actionId, body)
    if not ok then return false, err end

    db():QueryTyped(
        "INSERT INTO Message (FromIdentity, ToUserId, ToUsername, Body, ActionId, Verified) VALUES (?, ?, ?, ?, ?, 1);",
        from, recipient.Id, recipient.Name, body, actionId)
    return true
end

-- Server list aggregation

local peerServersCache = { time = 0, list = {} }

function fed.AggregatePeerServers()
    if os.time() - peerServersCache.time < 15 then return peerServersCache.list end
    local list = {}
    for _, peer in ipairs(fed.GetPeers()) do
        local res = httpGet(rstrip(peer.BaseUrl) .. "/fed/v1/servers")
        for _, s in ipairs(asList(res and parseJson(res.Body))) do
            s.PeerDomain, s.PeerBaseUrl = peer.Domain, peer.BaseUrl
            list[#list + 1] = s
        end
    end
    peerServersCache = { time = os.time(), list = list }
    return list
end

-- Forums

function fed.ForumCanonical(action)
    if action.Type == "thread" then
        return table.concat({ "thread", action.ForumId, action.Subject or "", action.Content or "" }, "\n")
    end
    return table.concat({ "reply", action.ThreadId, action.Content or "" }, "\n")
end

local function validateForumTarget(m, action)
    if blank(action.Content) then return false, "content is empty" end
    if action.Type == "thread" then
        if not firstRow(m:QueryTyped("SELECT 1 FROM Forum WHERE Id = ?;", action.ForumId)) then return false, "no such forum" end
        if blank(action.Subject) then return false, "subject is empty" end
    elseif action.Type == "reply" then
        if not firstRow(m:QueryTyped("SELECT 1 FROM ForumThread WHERE Id = ?;", action.ThreadId)) then return false, "no such thread" end
    else
        return false, "unknown post type"
    end
    return true
end

local function insertForumPost(m, action, posterId)
    local threadId = action.ThreadId
    if action.Type == "thread" then
        m:QueryTyped("INSERT INTO ForumThread (Subject, Created, Poster, ForumId) VALUES (?, unixepoch(), ?, ?);",
            action.Subject, posterId, action.ForumId)
        threadId = firstRow(m:Query("SELECT last_insert_rowid() AS Id;")).Id
    end
    m:QueryTyped("INSERT INTO ForumPost (ThreadId, Created, Poster, Content) VALUES (?, unixepoch(), ?, ?);",
        threadId, posterId, action.Content)
    m:MarkDirty()
    return threadId
end

local function findOrCreateFederatedAuthor(m, identity)
    local existing = firstRow(m:QueryTyped("SELECT Id FROM User WHERE Name = ? COLLATE NOCASE;", identity))
    if existing then return existing.Id end
    m:QueryTyped("INSERT INTO User (Name, DisplayName, JoinDate) VALUES (?, ?, unixepoch());", identity, identity)
    local row = firstRow(m:Query("SELECT last_insert_rowid() AS Id;"))
    return row and row.Id
end

-- This server's public forum tree, or one thread's posts when threadId is given.
function fed.LocalForumTree(threadId)
    local m = core.GetMasterDatabase()
    if m == nil then return nil end

    if threadId then
        local thread = firstRow(m:QueryTyped("SELECT Id, Subject, ForumId FROM ForumThread WHERE Id = ?;", threadId))
        if not thread then return { Thread = nil, Posts = {} } end
        local posts = m:QueryTyped([[
            SELECT p.Created AS Created, p.Content AS Content,
                   COALESCE(u.DisplayName, u.Name) AS PosterName, u.Name AS PosterUserName
            FROM ForumPost p JOIN User u ON p.Poster = u.Id
            WHERE p.ThreadId = ? ORDER BY p.Created ASC, p.Id ASC;]], threadId)
        return { Thread = thread, Posts = asList(posts) }
    end

    local tree = {}
    for _, cat in ipairs(asList(m:Query("SELECT Id, Name FROM ForumCategory ORDER BY SortOrder DESC;"))) do
        local forums = {}
        for _, forum in ipairs(asList(m:QueryTyped(
            "SELECT Id, Name, Description FROM Forum WHERE CategoryId = ? ORDER BY SortOrder DESC;", cat.Id))) do
            forum.Threads = asList(m:QueryTyped([[
                SELECT t.Id AS Id, t.Subject AS Subject,
                       COALESCE(u.DisplayName, u.Name) AS PosterName, u.Name AS PosterUserName,
                       (SELECT COUNT(*) FROM ForumPost p WHERE p.ThreadId = t.Id) AS PostCount
                FROM ForumThread t JOIN User u ON t.Poster = u.Id
                WHERE t.ForumId = ? ORDER BY t.Created DESC LIMIT 25;]], forum.Id))
            -- Per-forum summary for the index table (thread/post counts + last post).
            local stats = firstRow(m:QueryTyped([[
                SELECT (SELECT COUNT(*) FROM ForumThread WHERE ForumId = ?) AS ThreadCount,
                       (SELECT COUNT(*) FROM ForumPost p JOIN ForumThread t ON p.ThreadId = t.Id
                        WHERE t.ForumId = ?) AS PostCount;]], forum.Id, forum.Id))
            forum.ThreadCount = stats and tonumber(stats.ThreadCount) or 0
            forum.PostCount   = stats and tonumber(stats.PostCount) or 0
            local last = firstRow(m:QueryTyped([[
                SELECT p.Created AS Created,
                       COALESCE(u.DisplayName, u.Name) AS PosterName, u.Name AS PosterUserName
                FROM ForumPost p JOIN ForumThread t ON p.ThreadId = t.Id JOIN User u ON p.Poster = u.Id
                WHERE t.ForumId = ? ORDER BY p.Created DESC, p.Id DESC LIMIT 1;]], forum.Id))
            if last then
                forum.LastPost = { Created = last.Created, PosterName = last.PosterName,
                                   PosterUserName = last.PosterUserName }
            end
            forums[#forums + 1] = forum
        end
        tree[#tree + 1] = { Name = cat.Name, Forums = forums }
    end
    return { Categories = tree }
end

function fed.SendForumPost(fromUserId, fromUsername, peerDomain, action)
    if peerDomain == nil or fed.IsLocalDomain(peerDomain) then return false, "that is your own server" end
    if blank(action.Content) then return false, "content is empty" end

    local actionId = hash.GenerateToken()
    if not recordOutbound(actionId, fromUserId, fromUsername, "forum:" .. peerDomain, fed.ForumCanonical(action)) then
        return false, "failed to record outbound post"
    end

    local payload = { From = _G.MASTERSERVER_FULL_IDENTITY(fromUsername), Type = action.Type, Content = action.Content, ActionId = actionId }
    if action.Type == "thread" then
        payload.ForumId, payload.Subject = action.ForumId, action.Subject
    else
        payload.ThreadId = action.ThreadId
    end
    return deliver(peerDomain, "/fed/v1/forum-post", payload)
end

function fed.ReceiveForumPost(from, action, actionId)
    if not fed.ParseIdentity(from) then return false, "malformed sender identity" end
    if blank(actionId) then return false, "missing action id" end
    if fed.ActionSeen(actionId) then return false, "duplicate post" end

    local m = core.GetMasterDatabase()
    if m == nil then return false, "no master database" end

    local ok, err = validateForumTarget(m, action)
    if not ok then return false, err end
    ok, err = fed.VerifyRemoteAction(from, actionId, fed.ForumCanonical(action))
    if not ok then return false, err end

    local authorId = findOrCreateFederatedAuthor(m, from)
    if not authorId then return false, "could not record author" end
    insertForumPost(m, action, authorId)
    fed.MarkActionSeen(actionId, "forum")
    return true
end

-- Posts to one of this server's own forums (a normal EmuDb write). Returns ok, threadId.
function fed.PostLocalForum(userId, action)
    local m = core.GetMasterDatabase()
    if m == nil then return false, "no master database" end
    local ok, err = validateForumTarget(m, action)
    if not ok then return false, err end
    return true, insertForumPost(m, action, userId)
end

local peerForumsCache = { time = 0, list = {} }

function fed.AggregatePeerForums()
    if os.time() - peerForumsCache.time < 30 then return peerForumsCache.list end
    local list = {}
    for _, peer in ipairs(fed.GetPeers()) do
        local res = httpGet(rstrip(peer.BaseUrl) .. "/fed/v1/forums")
        local tree = res and parseJson(res.Body)
        if tree and tree.Categories then list[#list + 1] = { Peer = peer, Tree = tree } end
    end
    peerForumsCache = { time = os.time(), list = list }
    return list
end

function fed.FetchPeerThread(peerDomain, threadId)
    local res = httpGet(fed.ResolveBaseUrl(peerDomain) .. "/fed/v1/forums?thread=" .. tostring(tonumber(threadId) or 0))
    return res and parseJson(res.Body)
end

-- "My Feed" statuses
function fed.PostStatus(identity, body)
    if blank(body) then return false, "status is empty" end
    db():QueryTyped("INSERT INTO Status (AuthorIdentity, Body) VALUES (?, ?);", identity, tostring(body))
    local row = firstRow(db():Query("SELECT last_insert_rowid() AS Id;"))
    return true, row and row.Id
end

function fed.LocalStatuses(limit)
    local rows = db():QueryTyped([[
        SELECT s.Id AS Id, s.AuthorIdentity AS AuthorIdentity, s.Body AS Body, s.Created AS Created,
               (SELECT COUNT(*) FROM Status r WHERE r.ParentId = s.Id) AS ReplyCount
        FROM Status s WHERE s.ParentId IS NULL ORDER BY s.Created DESC LIMIT ?;]], tonumber(limit) or 50)
    return asList(rows)
end

function fed.LocalStatusThread(statusId)
    local status = firstRow(db():QueryTyped(
        "SELECT Id, AuthorIdentity, Body, Created FROM Status WHERE Id = ? AND ParentId IS NULL;", statusId))
    if not status then return nil end
    local replies = db():QueryTyped(
        "SELECT AuthorIdentity, Body, Created FROM Status WHERE ParentId = ? ORDER BY Created ASC;", statusId)
    return { Status = status, Replies = asList(replies) }
end

function fed.StatusesByAuthor(identity, limit)
    local rows = db():QueryTyped(
        "SELECT Id, Body, Created, ParentId FROM Status WHERE AuthorIdentity = ? ORDER BY Created DESC LIMIT ?;",
        identity, tonumber(limit) or 20)
    return asList(rows)
end

local function feedItem(origin, s)
    return { Origin = origin, StatusId = s.Id, AuthorIdentity = s.AuthorIdentity,
             Body = s.Body, Created = s.Created, ReplyCount = s.ReplyCount }
end

-- Local statuses
local feedPeerCache = { time = 0, list = {} }

function fed.AggregateFeed(limit)
    limit = tonumber(limit) or 50
    local list = {}
    for _, s in ipairs(fed.LocalStatuses(limit)) do list[#list + 1] = feedItem("local", s) end

    if os.time() - feedPeerCache.time >= 15 then
        fed.GossipPeers()
        local peerList = {}
        for _, peer in ipairs(fed.GetPeers()) do
            local res = httpGet(rstrip(peer.BaseUrl) .. "/fed/v1/statuses")
            local data = res and parseJson(res.Body)
            if data and type(data.Statuses) == "table" then
                for _, s in ipairs(data.Statuses) do peerList[#peerList + 1] = feedItem(peer.Domain, s) end
            end
        end
        feedPeerCache = { time = os.time(), list = peerList }
    end
    for _, s in ipairs(feedPeerCache.list) do list[#list + 1] = s end

    table.sort(list, function(a, b) return (tonumber(a.Created) or 0) > (tonumber(b.Created) or 0) end)
    local trimmed = {}
    for i = 1, math.min(#list, limit) do trimmed[i] = list[i] end
    return trimmed
end

function fed.FetchPeerStatusThread(domain, statusId)
    local res = httpGet(fed.ResolveBaseUrl(domain) .. "/fed/v1/statuses?status=" .. tostring(tonumber(statusId) or 0))
    return res and parseJson(res.Body)
end

local function statusReplyCanonical(parentStatusId, body)
    return table.concat({ "status-reply", parentStatusId, body }, "\n")
end

function fed.ReplyToStatus(fromUserId, fromUsername, parentOrigin, parentStatusId, body)
    if blank(body) then return false, "reply is empty" end
    body = tostring(body)
    parentStatusId = tonumber(parentStatusId)
    if not parentStatusId then return false, "invalid parent status" end

    if parentOrigin == nil or parentOrigin == "local" or fed.IsLocalDomain(parentOrigin) then
        if not firstRow(db():QueryTyped("SELECT 1 FROM Status WHERE Id = ? AND ParentId IS NULL;", parentStatusId)) then
            return false, "that status doesn't exist"
        end
        db():QueryTyped("INSERT INTO Status (AuthorIdentity, Body, ParentId) VALUES (?, ?, ?);",
            _G.MASTERSERVER_FULL_IDENTITY(fromUsername), body, parentStatusId)
        return true
    end

    local actionId = hash.GenerateToken()
    if not recordOutbound(actionId, fromUserId, fromUsername, "status:" .. parentOrigin,
        statusReplyCanonical(parentStatusId, body)) then
        return false, "failed to record outbound reply"
    end
    fed.AutoAddPeer(parentOrigin, nil)
    return deliver(parentOrigin, "/fed/v1/status-reply",
        { From = _G.MASTERSERVER_FULL_IDENTITY(fromUsername), ParentStatusId = parentStatusId, Body = body, ActionId = actionId })
end

function fed.ReceiveStatusReply(from, parentStatusId, body, actionId)
    if not fed.ParseIdentity(from) then return false, "malformed sender identity" end
    if blank(actionId) then return false, "missing action id" end
    if fed.ActionSeen(actionId) then return false, "duplicate reply" end
    parentStatusId = tonumber(parentStatusId)
    if not parentStatusId then return false, "invalid parent status" end
    if blank(body) then return false, "empty reply" end
    body = tostring(body)

    if not firstRow(db():QueryTyped("SELECT 1 FROM Status WHERE Id = ? AND ParentId IS NULL;", parentStatusId)) then
        return false, "no such status"
    end

    local ok, err = fed.VerifyRemoteAction(from, actionId, statusReplyCanonical(parentStatusId, body))
    if not ok then return false, err end

    db():QueryTyped("INSERT INTO Status (AuthorIdentity, Body, ParentId, ActionId) VALUES (?, ?, ?, ?);",
        from, body, parentStatusId, actionId)
    fed.MarkActionSeen(actionId, "status-reply")
    return true
end

return fed
