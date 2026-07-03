-- ////////////////////////////////////////////////////////////////////////////////
-- noobWarrior
-- Plugin: Master Server
-- File: federation.lua
-- Description: A bunch of functions that handle federation between master servers
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

-- Defederation: a block-list of banned domains kept as a list in master.federation.banned.

local function bannedSet()
    local list = reg.GetKeyValue("master.federation.banned")
    local set = {}
    if type(list) == "table" then
        for _, d in ipairs(list) do
            d = tostring(d):gsub("^%s*(.-)%s*$", "%1"):lower()
            if d ~= "" then set[d] = true end
        end
    end
    return set
end

local function writeBanned(set)
    local list = {}
    for d in pairs(set) do list[#list + 1] = d end
    reg.SetKeyValue("master.federation.banned", list)
end

function fed.IsBanned(domain)
    if blank(domain) then return false end
    return bannedSet()[tostring(domain):lower()] == true
end

function fed.BanPeer(domain)
    domain = tostring(domain or ""):gsub("^%s*(.-)%s*$", "%1"):lower()
    if domain == "" or fed.IsLocalDomain(domain) then return false end
    local set = bannedSet()
    set[domain] = true
    writeBanned(set)
    db():QueryTyped("UPDATE Peer SET Status = 'banned' WHERE Domain = ? COLLATE NOCASE;", domain)
    return true
end

function fed.UnbanPeer(domain)
    domain = tostring(domain or ""):gsub("^%s*(.-)%s*$", "%1"):lower()
    if domain == "" then return false end
    local set = bannedSet()
    set[domain] = nil
    writeBanned(set)
    db():QueryTyped("UPDATE Peer SET Status = 'active' WHERE Domain = ? COLLATE NOCASE;", domain)
    return true
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
    if baseUrl == "" then
        return nil, "empty url"
    end

    local res, err = httpGet(baseUrl .. "/fed/v1/info")
    if not res then
        return nil, "could not reach peer: " .. tostring(err)
    end
    local info = parseJson(res.Body)
    if not info or not info.Domain then
        return nil, "peer did not return valid federation info"
    end

    local domain = tostring(info.Domain)
    if fed.IsLocalDomain(domain) then
        return nil, "that peer is this server"
    end
    local name = tostring(info.Name or domain)
    -- Pin the peer's Ed25519 key. A manual (re-)add trusts whatever key it currently presents, so this
    -- is also how an operator rotates a peer's key after it regenerates one.
    local pubKey = tostring(info.PublicKey or "")

    if peerKnown(domain) then
        db():QueryTyped(
            "UPDATE Peer SET BaseUrl = ?, Name = ?, PublicKey = ?, LastSeen = unixepoch(), Status = 'active' WHERE Domain = ? COLLATE NOCASE;",
            baseUrl, name, pubKey, domain)
    else
        db():QueryTyped("INSERT INTO Peer (Domain, BaseUrl, Name, PublicKey) VALUES (?, ?, ?, ?);",
            domain, baseUrl, name, pubKey)
    end
    return { Domain = domain, BaseUrl = baseUrl, Name = name }
end

function fed.AutoAddPeer(domain, baseUrl)
    if domain == nil or fed.IsLocalDomain(domain) or fed.IsBanned(domain) or peerKnown(domain) then
        return
    end
    baseUrl = blank(baseUrl) and ("https://" .. domain) or rstrip(baseUrl)
    db():QueryTyped("INSERT INTO Peer (Domain, BaseUrl, Name) VALUES (?, ?, ?);", domain, baseUrl, domain)
end

function fed.AutoAccept(fromIdentity, baseUrl)
    if not fed.AutoEnabled() then
        return
    end
    local _, domain = fed.ParseIdentity(fromIdentity)
    fed.AutoAddPeer(domain, baseUrl)
end

function fed.GossipPeers()
    if not fed.AutoEnabled() then
        return
    end
    for _, peer in ipairs(fed.GetPeers()) do
        local res = httpGet(rstrip(peer.BaseUrl) .. "/fed/v1/peers")
        local data = res and parseJson(res.Body)
        if data and type(data.Peers) == "table" then
            for _, p in ipairs(data.Peers) do
                fed.AutoAddPeer(p.Domain, p.BaseUrl)
            end
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

-- A peer's pinned Ed25519 public key (hex), or nil. On first contact we fetch it from the peer's
-- /fed/v1/info and pin it (trust-on-first-use); an already-pinned key is never silently replaced
-- (re-add the peer to rotate it), so a later MITM can't swap a peer's key out from under us.
function fed.PeerPublicKey(domain)
    local row = firstRow(db():QueryTyped("SELECT PublicKey FROM Peer WHERE Domain = ? COLLATE NOCASE;", domain))
    if row and not blank(row.PublicKey) then return row.PublicKey end

    local res = httpGet(fed.ResolveBaseUrl(domain) .. "/fed/v1/info")
    local info = res and parseJson(res.Body)
    if not info or blank(info.PublicKey) then return nil end
    if peerKnown(domain) then
        db():QueryTyped(
            "UPDATE Peer SET PublicKey = ? WHERE Domain = ? COLLATE NOCASE AND (PublicKey IS NULL OR PublicKey = '');",
            info.PublicKey, domain)
    else
        db():QueryTyped("INSERT INTO Peer (Domain, BaseUrl, Name, PublicKey) VALUES (?, ?, ?, ?);",
            domain, fed.ResolveBaseUrl(domain), domain, info.PublicKey)
    end
    return info.PublicKey
end

-- Every federated action is signed over these fixed, newline-joined fields. Binding the actionId, the
-- signer's identity and the recipient domain into the signature stops replay against a different target.
function fed.SigningEnvelope(actionId, fromIdentity, targetDomain, body)
    return table.concat({ "nwfed1", tostring(actionId), tostring(fromIdentity):lower(),
                          tostring(targetDomain):lower(), tostring(body) }, "\n")
end

-- Sign an outbound action with this master's private key. targetDomain is the RECIPIENT's domain.
function fed.SignAction(actionId, fromIdentity, targetDomain, body)
    if blank(_G.MASTERSERVER_PRIVKEY) then return "" end
    return crypto.Sign(_G.MASTERSERVER_PRIVKEY, fed.SigningEnvelope(actionId, fromIdentity, targetDomain, body))
end

-- Verify a signed federated action against the signer domain's pinned key. No callback to the origin:
-- the signature is self-contained, so the issuer needn't be reachable. expectedTarget defaults to this
-- server (the action must have been signed FOR us). A master can only vouch for its own users, because
-- a foreign identity resolves to a different domain's key that this signature won't verify against.
function fed.VerifyRemoteActionFor(fromIdentity, actionId, body, expectedTarget, signature)
    local _, domain = fed.ParseIdentity(fromIdentity)
    if not domain then return false, "malformed sender identity" end
    if fed.IsBanned(domain) then return false, "origin is defederated" end
    if blank(signature) then return false, "missing signature" end

    local pub = fed.PeerPublicKey(domain)
    if blank(pub) then return false, "no public key for " .. domain end

    local target = tostring(expectedTarget or _G.MASTERSERVER_DOMAIN()):lower()
    if not crypto.Verify(pub, fed.SigningEnvelope(actionId, fromIdentity, target, body), tostring(signature)) then
        return false, "bad signature"
    end
    return true
end

function fed.VerifyRemoteAction(fromIdentity, actionId, body, signature)
    return fed.VerifyRemoteActionFor(fromIdentity, actionId, body, _G.MASTERSERVER_DOMAIN(), signature)
end

-- Federated join vouchers. A player's home master mints a one-time voucher bound to the slave's
-- master (target), which then verifies it via origin-callback before admitting the player.

function fed.JoinCanonical(identity, targetDomain, nonce)
    return table.concat({ "join", tostring(identity), tostring(targetDomain):lower(), tostring(nonce) }, "\n")
end

-- True if targetUrl points back at THIS master (a slave may set emu.auth.master to its own master).
-- We then resolve the domain locally instead of an HTTP call to ourselves, which stalls the loop.
local function isSelfMaster(url)
    local scheme, host, port = tostring(url):match("^(%w+)://([^:/]+):?(%d*)")
    if not host then return false end
    host = host:lower()
    if host ~= "127.0.0.1" and host ~= "localhost" and host ~= "::1" then return false end
    port = tonumber(port) or (scheme == "https" and 443 or 80)
    return port == (tonumber(reg.GetKeyValue("master.http_port")) or 80)
        or port == (tonumber(reg.GetKeyValue("master.https_port")) or 443)
end

-- Runs on the player's HOME master. user is the authenticated local user row {Id, Name}; targetUrl is
-- the slave's master URL. Returns { actionId, identity, body } or nil, err.
function fed.MintJoinVoucher(user, targetUrl)
    if not user or blank(user.Name) then return nil, "not signed in" end
    if blank(targetUrl) then return nil, "missing target master url" end

    local targetDomain
    if isSelfMaster(targetUrl) then
        targetDomain = _G.MASTERSERVER_DOMAIN():lower()
    else
        local res, err = httpGet(rstrip(targetUrl) .. "/fed/v1/info")
        if not res then return nil, "could not reach target master: " .. tostring(err) end
        local info = parseJson(res.Body)
        if not info or blank(info.Domain) then return nil, "target is not a master server" end
        targetDomain = tostring(info.Domain):lower()
    end
    if fed.IsBanned(targetDomain) then return nil, "you have defederated that server" end

    local identity = _G.MASTERSERVER_FULL_IDENTITY(user.Name)
    local nonce = hash.GenerateToken()
    local body = fed.JoinCanonical(identity, targetDomain, nonce)
    local actionId = hash.GenerateToken()
    if not recordOutbound(actionId, user.Id, user.Name, targetDomain, body) then
        return nil, "failed to record voucher"
    end
    local signature = fed.SignAction(actionId, identity, targetDomain, body)
    if blank(signature) then return nil, "failed to sign voucher" end
    return { actionId = actionId, identity = identity, body = body, signature = signature }
end

-- Runs on the slave's master. Confirms a voucher and returns the join identity, or nil, err.
-- allowForeign gates identities from OTHER masters; our own users are always allowed.
function fed.VerifyFederatedJoin(identity, actionId, body, allowForeign, signature)
    local username, domain = fed.ParseIdentity(identity)
    if not username then return nil, "malformed identity" end
    if blank(actionId) then return nil, "missing action id" end
    if fed.IsBanned(domain) then return nil, "that master server is defederated" end
    if not fed.IsLocalDomain(domain) and allowForeign == false then
        return nil, "this server only accepts logins from its own master server"
    end
    if fed.ActionSeen(actionId) then return nil, "voucher already used" end
    body = tostring(body or "")

    if fed.IsLocalDomain(domain) then
        -- Our own user: verify the voucher we recorded without an HTTP round-trip to ourselves.
        local out = fed.LookupOutbound(actionId)
        if not out then return nil, "unknown voucher" end
        if tostring(out.ToIdentity):lower() ~= _G.MASTERSERVER_DOMAIN():lower() then return nil, "voucher not for this server" end
        if _G.MASTERSERVER_FULL_IDENTITY(out.FromUsername):lower() ~= identity:lower() then return nil, "actor mismatch" end
        if tostring(out.BodyHash or "") ~= hash.Sha256(body) then return nil, "body hash mismatch" end
    else
        if not peerKnown(domain) and not fed.AutoEnabled() then return nil, "that master server is not federated" end
        local ok, err = fed.VerifyRemoteActionFor(identity, actionId, body, _G.MASTERSERVER_DOMAIN(), signature)
        if not ok then return nil, err end
    end

    fed.MarkActionSeen(actionId, "join")
    -- Where the slave can fetch this user's avatar (their home master serves it over /fed/v1/avatar).
    local homeBaseUrl = fed.IsLocalDomain(domain) and fed.SelfBaseUrl() or fed.ResolveBaseUrl(domain)
    return {
        id = _G.MASTERSERVER_ONLINE_USER_ID(identity),
        name = identity,
        displayName = username,
        homeBaseUrl = homeBaseUrl,
    }
end

-- Serves a local user's avatar-fetch JSON by handle (for a federated slave). nil if not our user.
function fed.LocalAvatarJson(handle)
    local username, domain = fed.ParseIdentity(handle)
    if not username then username = tostring(handle or "") end
    if domain and not fed.IsLocalDomain(domain) then return nil end
    local m = core.GetMasterDatabase()
    local row = m and firstRow(m:QueryTyped("SELECT Id FROM User WHERE Name = ? COLLATE NOCASE;", username))
    if not row then return nil end
    return core.BuildAvatarFetchJson(row.Id)
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
    local signature = fed.SignAction(actionId, fromIdentity, toDomain, body)
    return deliver(toDomain, "/fed/v1/inbox",
        { From = fromIdentity, To = toUser, Body = body, ActionId = actionId, Signature = signature })
end

function fed.ReceiveMessage(from, toUsername, body, actionId, signature)
    if not fed.ParseIdentity(from) then return false, "malformed sender identity" end
    if blank(actionId) then return false, "missing action id" end
    if blank(toUsername) then return false, "missing recipient" end
    body = tostring(body or "")

    local recipient = localUserRow(toUsername)
    if not recipient then return false, "no such user on this server" end
    if firstRow(db():QueryTyped("SELECT 1 FROM Message WHERE ActionId = ? LIMIT 1;", actionId)) then
        return false, "duplicate message"
    end

    local ok, err = fed.VerifyRemoteAction(from, actionId, body, signature)
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
    if peerDomain == nil or fed.IsLocalDomain(peerDomain) then
        return false, "that is your own server"
    end
    if blank(action.Content) then
        return false, "content is empty"
    end

    local actionId = hash.GenerateToken()
    local canonical = fed.ForumCanonical(action)
    if not recordOutbound(actionId, fromUserId, fromUsername, "forum:" .. peerDomain, canonical) then
        return false, "failed to record outbound post"
    end

    local fromIdentity = _G.MASTERSERVER_FULL_IDENTITY(fromUsername)
    local payload = { From = fromIdentity, Type = action.Type, Content = action.Content, ActionId = actionId,
                      Signature = fed.SignAction(actionId, fromIdentity, peerDomain, canonical) }
    if action.Type == "thread" then
        payload.ForumId, payload.Subject = action.ForumId, action.Subject
    else
        payload.ThreadId = action.ThreadId
    end
    return deliver(peerDomain, "/fed/v1/forum-post", payload)
end

function fed.ReceiveForumPost(from, action, actionId, signature)
    if not fed.ParseIdentity(from) then
        return false, "malformed sender identity"
    end
    if blank(actionId) then
        return false, "missing action id"
    end
    if fed.ActionSeen(actionId) then
        return false, "duplicate post"
    end

    local m = core.GetMasterDatabase()
    if m == nil then
        return false, "no master database"
    end

    local ok, err = validateForumTarget(m, action)
    if not ok then
        return false, err
    end
    ok, err = fed.VerifyRemoteAction(from, actionId, fed.ForumCanonical(action), signature)
    if not ok then
        return false, err
    end

    local authorId = findOrCreateFederatedAuthor(m, from)
    if not authorId then
        return false, "could not record author"
    end
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
    local canonical = statusReplyCanonical(parentStatusId, body)
    if not recordOutbound(actionId, fromUserId, fromUsername, "status:" .. parentOrigin, canonical) then
        return false, "failed to record outbound reply"
    end
    fed.AutoAddPeer(parentOrigin, nil)
    local fromIdentity = _G.MASTERSERVER_FULL_IDENTITY(fromUsername)
    return deliver(parentOrigin, "/fed/v1/status-reply",
        { From = fromIdentity, ParentStatusId = parentStatusId, Body = body, ActionId = actionId,
          Signature = fed.SignAction(actionId, fromIdentity, parentOrigin, canonical) })
end

function fed.ReceiveStatusReply(from, parentStatusId, body, actionId, signature)
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

    local ok, err = fed.VerifyRemoteAction(from, actionId, statusReplyCanonical(parentStatusId, body), signature)
    if not ok then return false, err end

    db():QueryTyped("INSERT INTO Status (AuthorIdentity, Body, ParentId, ActionId) VALUES (?, ?, ?, ?);",
        from, body, parentStatusId, actionId)
    fed.MarkActionSeen(actionId, "status-reply")
    return true
end

return fed
