/*
 * Copyright (C) 2026 Hattozo
 *
 * This file is part of noobWarrior.
 *
 * noobWarrior is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * noobWarrior is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with noobWarrior; if not, see
 * <https://www.gnu.org/licenses/>.
 */
// === noobWarrior ===
// File: ServerEmulator.cpp
// Started by: Hattozo
// Started on: 9/2/2025
// Description:
#include <cpr/cpr.h>

#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/HttpServer/Emulator/ClientSettingsHandler.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Registry.h>
#include <NoobWarrior/Macros.h>

#include "NoobWarrior/HttpServer/Emulator/RequestAuthHandler.h"

#include <algorithm>
#include <atomic>
#include <charconv>
#include <chrono>
#include <ctime>
#include <mutex>
#include <thread>

using namespace NoobWarrior;
using json = nlohmann::json;

ServerEmulator::ServerEmulator(Core *core) : HttpServer(core, "ServerEmulator"),
    mProcessPingHandler(this),
    mCreateAccountHandler(this),
    mLoginHandler(this),
    mLogoutHandler(this),
    mRunningGameServersHandler(this),
    mAssetHandler(this, mCore->GetEmuDbManager()),
    mAssetThumbnailJsonHandler(this, mCore->GetEmuDbManager()),
    mClientSettingsHandler(this),
    mClientSettingsV2StudioHandler(this),
    mClientSettingsV2DesktopHandler(),
    mNegotiateHandler(),
    mPlaceLauncherHandler(this),
    mJoinScriptJsonHandler(this),
    mGameJoinHandler(this),
    mAuthTicketRedeemHandler(this),
    mAppBehaviorsHandler(),
    mAvatarRulesHandler(),
    mAvatarHandler(this),
    mAppLaunchInfoHandler(),
    mRolesHandler(),
    mLocalesHandler(),
    mGamesHandler(mCore->GetEmuDbManager()),
    mUserChannelHandler(),
    mPlaceDetailsHandler(mCore->GetEmuDbManager()),
    mPlaceUniverseHandler(this, mCore->GetEmuDbManager()),
    mToolboxServiceHandler(this, mCore->GetEmuDbManager()),
    mIdeToolboxHandler(mCore->GetEmuDbManager()),
    mDevelopHandler(this, mCore->GetEmuDbManager()),
    mDataUploadHandler(mCore->GetEmuDbManager()),
    mIdePublishHandler(this),
    mThumbnailHandler(mCore->GetEmuDbManager()),
    mOmniRecHandler(),
    mGamesSortsHandler(),
    mGamesListHandler(),
    mAvatarFetchHandler(this),
    mAvatarOverrideHandler(this),
    mAuthInfoHandler(this),
    mUniversalAppConfigStudioHandler(),
    mMySettingsJsonHandler(this),
    mAuthenticatedUserHandler(this),
    mSessionCheckHandler(this),
    mAvatarSetHandler(this),
    mAvatarCatalogHandler(this),
    mAvatarThumbnailHandler(this),
    mCurrentUserHandler(this),
    mRequestAuthHandler(),
    mStudioEditHandler(),
    mGameIconHandler(this, mCore->GetEmuDbManager()),
    mOAuthDiscoveryHandler(),
    mOAuthAuthorizeHandler(),
    mOAuthTokenHandler(this),
    mOAuthUserInfoHandler(this),
    mStudioLoginHandler(this),
    mStudioOpenPlaceHandler(this),
    mDataStorePersistenceHandler(this, mCore->GetEmuDbManager()),
    mEmulatorProxy(this),
    mAssetEnricher(mCore)
{
    mAssetEnricher.Start();
    StartAnnouncer();
}

ServerEmulator::~ServerEmulator() {
    StopAnnouncer();
    mAssetEnricher.Stop();
}

AssetEnricher* ServerEmulator::GetAssetEnricher() {
    return &mAssetEnricher;
}

void ServerEmulator::SetupHandlers() {
    HttpServer::SetupHandlers();

    SetRequestHandler("/v1/process-ping", &mProcessPingHandler);
    SetRequestHandler("/v1/create-account", &mCreateAccountHandler);
    SetRequestHandler("/v1/login", &mLoginHandler);
    SetRequestHandler("/v1/logout", &mLogoutHandler);
    SetRequestHandler("/v1/running-game-servers", &mRunningGameServersHandler);

    SetRequestHandler("/Asset", &mAssetHandler);
    SetRequestHandler("/asset", &mAssetHandler);
    SetRequestHandler("/asset/", &mAssetHandler);
    SetRequestHandler("/v1/asset", &mAssetHandler);
    SetRequestHandler("/v1/asset/", &mAssetHandler);

    SetRequestHandler("/asset-thumbnail/json", &mAssetThumbnailJsonHandler);

    SetRequestHandler("/v1/settings/application", &mClientSettingsHandler);
    SetRequestHandler("/v2/settings/application/PCDesktopClient", &mClientSettingsV2DesktopHandler);
    SetRequestHandler("/v2/settings/application/PCStudioApp", &mClientSettingsV2StudioHandler);

    SetRequestHandler("/Login/Negotiate.ashx", &mNegotiateHandler);
    SetRequestHandler("/login/negotiate.ashx", &mNegotiateHandler);

    SetRequestHandler("/Game/PlaceLauncher.ashx", &mPlaceLauncherHandler);
    SetRequestHandler("/game/placelauncher.ashx", &mPlaceLauncherHandler);

    SetRequestHandler("/Game/Join.ashx", &mJoinScriptJsonHandler);
    SetRequestHandler("/game/join.ashx", &mJoinScriptJsonHandler);

    SetRequestHandler("/gamejoin/v1/join-game", &mGameJoinHandler);
    SetRequestHandler("/v1/join-game", &mGameJoinHandler);
    SetRequestHandler("/universal-app-configuration/v1/behaviors/app-policy/content", &mAppBehaviorsHandler);
    SetRequestHandler("/universal-app-configuration/v1/behaviors/app-patch/content", &mAppBehaviorsHandler);
    SetRequestHandler("/v1/authentication-ticket/redeem", &mAuthTicketRedeemHandler);
    SetRequestHandler("/v1/avatar-rules", &mAvatarRulesHandler);
    SetRequestHandler("/v1/avatar", &mAvatarHandler);
    SetRequestHandler("/v1/users/authenticated/app-launch-info", &mAppLaunchInfoHandler);
    SetRequestHandler("/v1/users/authenticated/roles", &mRolesHandler);
    SetRequestHandler("/v1/locales/user-localization-locus-supported-locales", &mLocalesHandler);
    SetRequestHandler("/v1/games", &mGamesHandler);
    SetRequestHandler("/v2/user-channel", &mUserChannelHandler);
    SetRequestHandler("/v1/games/multiget-place-details", &mPlaceDetailsHandler);
    SetRequestHandler("/v1/games/multiget-playability-status", &mPlaceDetailsHandler);
    SetRequestHandler("/universes/v1/places/:placeId/universe", &mPlaceUniverseHandler);
    SetRequestHandler("/v1/places/:placeId/universe", &mPlaceUniverseHandler);

    SetRequestHandler("/v1/search/universes", &mDevelopHandler);
    SetRequestHandler("/v1/gametemplates", &mDevelopHandler);
    SetRequestHandler("/v1/user/groups/canmanage", &mDevelopHandler);
    SetRequestHandler("/v1/user/:userId/canmanage/:placeId", &mDevelopHandler);
    SetRequestHandler("/v1/universes/multiget/teamcreate", &mDevelopHandler);
    // Universe configuration — gates DataStore/HttpService access in Studio (isStudioAccessToApisAllowed).
    SetRequestHandler("/v1/universes/:universeId/configuration", &mDevelopHandler);
    SetRequestHandler("/v2/universes/:universeId/configuration", &mDevelopHandler);

    SetRequestHandler("/toolbox-service/v1/home/:assetType/configuration", &mToolboxServiceHandler);
    SetRequestHandler("/toolbox-service/v1/home/:typeId/section/:sectionName/assets", &mToolboxServiceHandler);
    SetRequestHandler("/toolbox-service/v1/items/details", &mToolboxServiceHandler);
    SetRequestHandler("/toolbox-service/v1/marketplace/:categoryId", &mToolboxServiceHandler);
    SetRequestHandler("/toolbox-service/v1/:category", &mToolboxServiceHandler);

    SetRequestHandler("/asset-permissions-api/v1/assets/check-permissions", &mAssetPermissionsHandler);

    SetRequestHandler("/IDE/Toolbox/Items.aspx", &mIdeToolboxHandler);
    SetRequestHandler("/ide/toolbox/items.aspx", &mIdeToolboxHandler);
    SetRequestHandler("/IDE/Toolbox/Items", &mIdeToolboxHandler);
    SetRequestHandler("/ide/toolbox/items", &mIdeToolboxHandler);
    SetRequestHandler("/IDE/ClientToolbox.aspx", &mIdeToolboxHandler);
    SetRequestHandler("/ide/clienttoolbox.aspx", &mIdeToolboxHandler);
    
    SetRequestHandler("/v1/batch", &mThumbnailHandler);
    SetRequestHandler("/emu-thumbnail", &mThumbnailHandler);
    SetRequestHandler("/discovery-api/omni-recommendation", &mOmniRecHandler);
    SetRequestHandler("/v1/games/sorts", &mGamesSortsHandler);
    SetRequestHandler("/v1/games/list", &mGamesListHandler);
    SetRequestHandler("/v2/avatar/avatar-fetch", &mAvatarFetchHandler);
    SetRequestHandler("/v1/avatar-fetch", &mAvatarFetchHandler);
    SetRequestHandler("/v1/avatar-fetch/", &mAvatarFetchHandler);
    SetRequestHandler("/v1.1/avatar-fetch", &mAvatarFetchHandler);
    SetRequestHandler("/v1.1/avatar-fetch/", &mAvatarFetchHandler);

    // Internal: a joining client federates its avatar appearance to the host here (see AvatarOverrideHandler).
    SetRequestHandler("/emu/v1/avatar-override", &mAvatarOverrideHandler);

    SetRequestHandler("/emu/v1/auth-info", &mAuthInfoHandler);
    SetRequestHandler("/emu/v1/session-check", &mSessionCheckHandler);
    SetRequestHandler("/emu/v1/avatar/mine", &mAvatarSetHandler);
    SetRequestHandler("/emu/v1/avatar/catalog", &mAvatarCatalogHandler);
    SetRequestHandler("/emu/v1/avatar/thumbnail", &mAvatarThumbnailHandler);

    SetRequestHandler("/universal-app-configuration/v1/behaviors/studio/content", &mUniversalAppConfigStudioHandler);

    SetRequestHandler("/my/settings/json", &mMySettingsJsonHandler);

    SetRequestHandler("/v1/users/authenticated", &mAuthenticatedUserHandler);

    SetRequestHandler("/game/GetCurrentUser.ashx", &mCurrentUserHandler);

    SetRequestHandler("/Login/RequestAuth.ashx", &mRequestAuthHandler);
    SetRequestHandler("/login/RequestAuth.ashx", &mRequestAuthHandler);

    SetRequestHandler("/Game/Edit.ashx", &mStudioEditHandler);
    SetRequestHandler("/game/edit.ashx", &mStudioEditHandler);

    SetRequestHandler("/Thumbs/GameIcon.ashx", &mGameIconHandler);
    SetRequestHandler("/Thumbs/gameicon.ashx", &mGameIconHandler);
    SetRequestHandler("/v1/games/icons", &mGameIconHandler); // home-grid square icons (batch list)
    
    SetRequestHandler("/Data/Upload.ashx", &mDataUploadHandler);
    SetRequestHandler("/data/upload.ashx", &mDataUploadHandler);
    
    SetRequestHandler("/ide/publish/uploadnewasset", &mIdePublishHandler);
    SetRequestHandler("/ide/publish/uploadexistingasset", &mIdePublishHandler);
    SetRequestHandler("/IDE/Publish/UploadNewAsset", &mIdePublishHandler);
    SetRequestHandler("/IDE/Publish/UploadExistingAsset", &mIdePublishHandler);

    SetRequestHandler("/oauth/.well-known/openid-configuration", &mOAuthDiscoveryHandler);
    SetRequestHandler("/oauth/v1/authorize", &mOAuthAuthorizeHandler);
    SetRequestHandler("/oauth/v1/token", &mOAuthTokenHandler);
    SetRequestHandler("/oauth/v1/userinfo", &mOAuthUserInfoHandler);
    SetRequestHandler("/studio-login/v1/login", &mStudioLoginHandler);

    SetRequestHandler("/studio-open-place/v1/openplace", &mStudioOpenPlaceHandler);
    
    SetRequestHandler("/v2/persistence/:universeId/datastores/objects/object/versions/version", &mDataStorePersistenceHandler);
    SetRequestHandler("/v2/persistence/:universeId/datastores/objects/object/versions", &mDataStorePersistenceHandler);
    SetRequestHandler("/v2/persistence/:universeId/datastores/objects/object/increment", &mDataStorePersistenceHandler);
    SetRequestHandler("/v2/persistence/:universeId/datastores/objects/object", &mDataStorePersistenceHandler);
    SetRequestHandler("/v2/persistence/:universeId/datastores/objects", &mDataStorePersistenceHandler);
    SetRequestHandler("/v2/persistence/:universeId/datastores", &mDataStorePersistenceHandler);

    SetRequestHandler("/persistence/getV2", &mDataStorePersistenceHandler);
    SetRequestHandler("/persistence/getv2", &mDataStorePersistenceHandler);
    SetRequestHandler("/persistence/set", &mDataStorePersistenceHandler);
    SetRequestHandler("/persistence/increment", &mDataStorePersistenceHandler);
    SetRequestHandler("/persistence/remove", &mDataStorePersistenceHandler);
    SetRequestHandler("/persistence/getSortedValues", &mDataStorePersistenceHandler);
    SetRequestHandler("/persistence/getsortedvalues", &mDataStorePersistenceHandler);
}

int ServerEmulator::Start(uint16_t port) {
    mAssetHandler.ResumeProxy();
    mEmulatorProxy.Resume();
    return HttpServer::Start(port);
}

int ServerEmulator::Stop() {
    // Must pause the proxy pools before HttpServer::Stop frees the evhttp, so no in-flight proxy
    // fetch/forward replies to a freed connection.
    mAssetHandler.PauseProxy();
    mEmulatorProxy.Pause();
    return HttpServer::Stop();
}

void ServerEmulator::PushProxyLayer(const std::string &host, uint16_t port, const std::string &sessionToken) {
    mEmulatorProxy.PushLayer(host, port, sessionToken);
}

bool ServerEmulator::PopProxyLayer() {
    return mEmulatorProxy.PopLayer();
}

void ServerEmulator::RemoveProxyLayer(const std::string &host, uint16_t port) {
    mEmulatorProxy.RemoveLayer(host, port);
}

void ServerEmulator::ClearProxyLayers() {
    mEmulatorProxy.ClearLayers();
}

std::vector<std::pair<std::string, uint16_t>> ServerEmulator::GetProxyLayers() const {
    std::vector<std::pair<std::string, uint16_t>> out;
    for (const auto &layer : mEmulatorProxy.GetLayers())
        out.emplace_back(layer.Host, layer.Port);
    return out;
}

bool ServerEmulator::TryProxyRequest(evhttp_request *req, std::function<void(evhttp_request *)> localFallback,
                                     EmulatorProxy::ResponseTransform responseTransform) {
    return mEmulatorProxy.TryProxy(req, std::move(localFallback), std::move(responseTransform));
}

void ServerEmulator::SetAvatarOverride(int64_t userId, const std::string &avatarFetchJson) {
    static constexpr size_t kMaxOverrideEntries = 4096;
    std::lock_guard lock(mAvatarOverridesMutex);
    if (mAvatarOverrides.size() >= kMaxOverrideEntries && !mAvatarOverrides.count(userId))
        return; // bound memory against a flood of new-user overrides
    mAvatarOverrides[userId] = avatarFetchJson;
    Out(mLogName, "Stored federated avatar override for userId={} ({} bytes)", userId, avatarFetchJson.size());
}

std::optional<std::string> ServerEmulator::GetAvatarOverride(int64_t userId) const {
    std::lock_guard lock(mAvatarOverridesMutex);
    if (auto it = mAvatarOverrides.find(userId); it != mAvatarOverrides.end())
        return it->second;
    return std::nullopt;
}

void ServerEmulator::ClearAvatarOverrides() {
    std::lock_guard lock(mAvatarOverridesMutex);
    mAvatarOverrides.clear();
}

void ServerEmulator::SetFederatedAvatarSource(int64_t userId, const std::string &sourceUrl) {
    if (sourceUrl.empty())
        return;
    static constexpr size_t kMaxEntries = 4096;
    std::lock_guard lock(mFederatedAvatarsMutex);
    if (mFederatedAvatars.size() >= kMaxEntries && !mFederatedAvatars.count(userId))
        return;
    mFederatedAvatars[userId] = { sourceUrl, "" }; // fresh source: drop any stale cached body
}

std::optional<std::string> ServerEmulator::GetFederatedAvatar(int64_t userId) {
    std::string sourceUrl;
    {
        std::lock_guard lock(mFederatedAvatarsMutex);
        auto it = mFederatedAvatars.find(userId);
        if (it == mFederatedAvatars.end())
            return std::nullopt; // not a federated joiner
        if (!it->second.CachedJson.empty())
            return it->second.CachedJson;
        sourceUrl = it->second.SourceUrl;
    }
    // Pull the avatar from the home master once, then cache it. Fetched outside the lock.
    cpr::Response res = cpr::Get(cpr::Url{sourceUrl}, cpr::Timeout{std::chrono::milliseconds(5000)},
                                 cpr::VerifySsl{false});
    if (res.error.code != cpr::ErrorCode::OK || res.status_code >= 400 || res.text.empty())
        return std::nullopt; // fall through to the local default avatar
    std::lock_guard lock(mFederatedAvatarsMutex);
    if (auto it = mFederatedAvatars.find(userId); it != mFederatedAvatars.end())
        it->second.CachedJson = res.text;
    return res.text;
}

bool ServerEmulator::BindFederatedHandle(int64_t userId, const std::string &handle) {
    static constexpr size_t kMaxEntries = 4096;
    std::lock_guard lock(mFederatedHandlesMutex);
    if (auto it = mFederatedHandles.find(userId); it != mFederatedHandles.end())
        return it->second == handle; // an id must always resolve to the same handle
    if (mFederatedHandles.size() >= kMaxEntries)
        return true; // never refuse a join just because the guard table filled up
    mFederatedHandles[userId] = handle;
    return true;
}

void ServerEmulator::SetCurrentLaunchUser(const AuthUtil::SessionUser &user) {
    std::lock_guard lock(mCurrentLaunchUserMutex);
    mCurrentLaunchUser = user;
}

std::optional<AuthUtil::SessionUser> ServerEmulator::GetCurrentLaunchUser() const {
    std::lock_guard lock(mCurrentLaunchUserMutex);
    return mCurrentLaunchUser;
}

std::optional<AuthUtil::SessionUser> ServerEmulator::ResolveJoiningUser(evhttp_request *req) {
    const char *peer = "";
    uint16_t peerPort = 0;
    if (evhttp_connection *conn = evhttp_request_get_connection(req))
        evhttp_connection_get_peer(conn, &peer, &peerPort);
    
    if (IsLoopbackOrEmpty(peer ? peer : "")) {
        if (auto launchUser = GetCurrentLaunchUser())
            return launchUser;
    }

    const char *cookie = evhttp_find_header(evhttp_request_get_input_headers(req), "Cookie");
    std::string session = AuthUtil::ExtractCookieValue(cookie, ".LOGINSESSION");

    // A remote joiner from a federated master forwards a voucher instead of a local session token.
    if (session.rfind("fedvoucher.", 0) == 0)
        return ResolveFederatedVoucher(session);

    EmuDb *master = mCore->GetEmuDbManager()->GetMasterDatabase();
    Registry *reg = mCore->GetRegistry();
    int64_t ttlSeconds = reg != nullptr
        ? reg->GetKeyValue<int64_t>("emu.auth.session_ttl_days").value_or(30) * 86400
        : 0;
    return AuthUtil::ResolveSessionUser(master, session, ttlSeconds);
}

std::optional<AuthUtil::SessionUser> ServerEmulator::ResolveFederatedVoucher(const std::string &cookieValue) {
    Registry *reg = mCore->GetRegistry();
    if (reg == nullptr)
        return std::nullopt;
    if (reg->GetKeyValue<std::string>("emu.auth.type").value_or("master") != "slave")
        return std::nullopt;
    std::string masterUrl = reg->GetKeyValue<std::string>("emu.auth.master").value_or("");
    if (masterUrl.empty())
        return std::nullopt;
    while (masterUrl.back() == '/')
        masterUrl.pop_back();
    // Our master accepts our own users always; only foreign federated masters are gated by this flag.
    bool allowForeign = reg->GetKeyValue<bool>("emu.auth.federated_login").value_or(true);

    // cookieValue = "fedvoucher.<b64url identity>.<actionId>.<b64url body>.<signature hex>"
    std::vector<std::string> parts;
    size_t start = 0;
    for (size_t dot = cookieValue.find('.'); dot != std::string::npos; dot = cookieValue.find('.', start)) {
        parts.push_back(cookieValue.substr(start, dot - start));
        start = dot + 1;
    }
    parts.push_back(cookieValue.substr(start));
    // 5 = signed (current); 4 = legacy unsigned, forwarded so the master returns a clear rejection.
    if (parts.size() != 5 && parts.size() != 4) {
        Out("ResolveFederatedVoucher", "Malformed voucher ({} segments, expected 5)", parts.size());
        return std::nullopt;
    }

    std::optional<std::string> identity = AuthUtil::Base64UrlDecode(parts[1]);
    std::optional<std::string> body = AuthUtil::Base64UrlDecode(parts[3]);
    if (!identity || !body) {
        Out("ResolveFederatedVoucher", "Voucher identity/body failed to base64url-decode");
        return std::nullopt;
    }

    json payload;
    payload["identity"] = *identity;
    payload["actionId"] = parts[2];
    payload["body"] = *body;
    payload["signature"] = parts.size() == 5 ? parts[4] : "";
    payload["allowForeign"] = allowForeign;

    cpr::Response res = cpr::Post(
        cpr::Url{masterUrl + "/v1/join/verify-federated"},
        cpr::Header{{"Content-Type", "application/json"}},
        cpr::Body{payload.dump()},
        cpr::Timeout{std::chrono::milliseconds(5000)},
        cpr::VerifySsl{false});
    if (res.error.code != cpr::ErrorCode::OK || res.status_code >= 400) {
        std::string reason;
        try { reason = json::parse(res.text).value("Error", ""); } catch (...) {}
        Out("ResolveFederatedVoucher", "Master {} refused {} (HTTP {}{}): {}", masterUrl,
            identity.value_or("?"), static_cast<long>(res.status_code),
            res.error.code != cpr::ErrorCode::OK ? " / network error" : "",
            reason.empty() ? res.error.message : reason);
        return std::nullopt;
    }

    try {
        json j = json::parse(res.text);
        if (!j.value("ok", false) || !j.contains("user")) {
            Out("ResolveFederatedVoucher", "Master {} did not confirm {}: {}", masterUrl,
                identity.value_or("?"), j.value("Error", "no user in response"));
            return std::nullopt;
        }
        const json &u = j["user"];
        AuthUtil::SessionUser user;
        // id arrives as a string (full-precision OnlineUserId) but tolerate a JSON number too.
        if (const json &idVal = u.value("id", json()); idVal.is_string()) {
            const std::string &s = idVal.get_ref<const std::string &>();
            std::from_chars(s.data(), s.data() + s.size(), user.id);
        } else if (idVal.is_number_integer()) {
            user.id = idVal.get<int64_t>();
        } else if (idVal.is_number_float()) {
            user.id = static_cast<int64_t>(idVal.get<double>());
        }
        user.name = u.value("name", "");
        user.displayName = u.value("displayName", "");
        if (user.displayName.empty())
            user.displayName = user.name;
        user.isFederated = true;
        if (user.id <= 0 || user.name.empty())
            return std::nullopt;

        // Reject if this OnlineUserId is already held by a different handle this run (identity collision).
        if (!BindFederatedHandle(user.id, user.name)) {
            Out("ServerEmulator", "Refused federated join: OnlineUserId {} already bound to a different handle (got \"{}\")",
                user.id, user.name);
            return std::nullopt;
        }

        // Remember where to pull this user's avatar from their home master, so AvatarFetchHandler can
        // serve it instead of the local default.
        std::string homeBaseUrl = u.value("homeBaseUrl", "");
        while (!homeBaseUrl.empty() && homeBaseUrl.back() == '/')
            homeBaseUrl.pop_back();
        if (!homeBaseUrl.empty())
            SetFederatedAvatarSource(user.id, homeBaseUrl + "/fed/v1/avatar?handle=" + cpr::util::urlEncode(user.name));
        return user;
    } catch (json::exception &) {
        return std::nullopt;
    }
}

void ServerEmulator::SetActiveEditDbFile(const std::string &dbFileName) {
    std::lock_guard lock(mActiveEditDbMutex);
    mActiveEditDbFile = dbFileName;
}

std::string ServerEmulator::GetActiveEditDbFile() const {
    std::lock_guard lock(mActiveEditDbMutex);
    return mActiveEditDbFile;
}

void ServerEmulator::SetMode(Mode mode) {
}

ServerEmulator::Mode ServerEmulator::GetMode() {
}

void ServerEmulator::SweepStaleInstancesLocked() {
    time_t now = std::time(nullptr);
    auto it = std::remove_if(mInstances.begin(), mInstances.end(),
        [&](const RunningInstance &inst) {
            bool stale = inst.LastSeen > 0 && (now - inst.LastSeen) > kStaleInstanceThresholdSecs;
            if (stale)
                Out(mLogName, "Reaping stale instance pid={} side={} (last seen {}s ago)",
                    inst.Pid, EngineSideAsString(inst.Side), static_cast<long long>(now - inst.LastSeen));
            return stale;
        });
    mInstances.erase(it, mInstances.end());
}

void ServerEmulator::RegisterInstance(const RunningInstance &instance) {
    std::lock_guard lock(mInstancesMutex);
    SweepStaleInstancesLocked();
    time_t now = std::time(nullptr);
    for (auto &existing : mInstances) {
        if (existing.Pid == instance.Pid) {
            existing = instance;
            existing.LastSeen = now;
            return;
        }
    }
    RunningInstance copy = instance;
    if (copy.FirstSeen == 0) copy.FirstSeen = now;
    copy.LastSeen = now;
    mInstances.push_back(copy);
    Out(mLogName, "Registered instance pid={} side={} version={}",
        copy.Pid, EngineSideAsString(copy.Side), copy.Version);
}

void ServerEmulator::UnregisterInstance(int pid) {
    std::lock_guard lock(mInstancesMutex);
    SweepStaleInstancesLocked();
    auto it = std::find_if(mInstances.begin(), mInstances.end(),
        [pid](const RunningInstance &i) { return i.Pid == pid; });
    if (it != mInstances.end()) {
        Out(mLogName, "Unregistered instance pid={} side={}", it->Pid, EngineSideAsString(it->Side));
        mInstances.erase(it);
    }
}

bool ServerEmulator::TouchInstance(int pid) {
    std::lock_guard lock(mInstancesMutex);
    SweepStaleInstancesLocked();
    auto it = std::find_if(mInstances.begin(), mInstances.end(),
        [pid](const RunningInstance &i) { return i.Pid == pid; });
    if (it == mInstances.end())
        return false;
    it->LastSeen = std::time(nullptr);
    return true;
}

std::vector<RunningInstance> ServerEmulator::GetRunningInstances() const {
    std::lock_guard lock(mInstancesMutex);
    const_cast<ServerEmulator*>(this)->SweepStaleInstancesLocked();
    return mInstances;
}

std::vector<RunningInstance> ServerEmulator::GetRunningGameServers() const {
    std::lock_guard lock(mInstancesMutex);
    const_cast<ServerEmulator*>(this)->SweepStaleInstancesLocked();
    std::vector<RunningInstance> servers;
    for (const auto &inst : mInstances) {
        if (inst.Side == EngineSide::Server)
            servers.push_back(inst);
    }
    return servers;
}

namespace {
std::mutex gPublicIpMtx;
std::string gPublicIp; // last successfully detected WAN IP
time_t gPublicIpFetchedAt = 0;
std::atomic<bool> gPublicIpFetching{false};

std::string DetectPublicIpBlocking() {
    cpr::Session session;
    session.SetUrl(cpr::Url{"https://api.ipify.org"});
    session.SetTimeout(cpr::Timeout{ 3000 });
    curl_easy_setopt(session.GetCurlHolder()->handle, CURLOPT_SSL_OPTIONS, (long)CURLSSLOPT_NATIVE_CA);
    cpr::Response r = session.Get();
    if (r.error.code != cpr::ErrorCode::OK || r.status_code != 200)
        return "";
    std::string ip = r.text;
    ip.erase(0, ip.find_first_not_of(" \t\r\n"));
    if (auto end = ip.find_last_not_of(" \t\r\n"); end != std::string::npos)
        ip.erase(end + 1);
    return ip;
}

std::string CachedPublicIp() {
    std::string current;
    bool needRefresh;
    {
        std::lock_guard<std::mutex> lock(gPublicIpMtx);
        current = gPublicIp;
        needRefresh = gPublicIp.empty() || std::time(nullptr) - gPublicIpFetchedAt > 3600;
    }
    if (needRefresh && !gPublicIpFetching.exchange(true)) {
        std::thread([] {
            std::string detected = DetectPublicIpBlocking();
            if (!detected.empty()) {
                std::lock_guard<std::mutex> lock(gPublicIpMtx);
                gPublicIp = detected;
                gPublicIpFetchedAt = std::time(nullptr);
            }
            gPublicIpFetching = false;
        }).detach();
    }
    return current;
}
}

std::string ServerEmulator::ResolveAdvertisedAddress(const std::string &localAddr) {
    std::string advertised = mCore->GetRegistry()
        ->GetKeyValue<std::string>("emu.public_ip").value_or("");
    if (advertised.empty()) advertised = CachedPublicIp();
    if (advertised.empty()) advertised = localAddr;
    return advertised;
}

void ServerEmulator::StartAnnouncer() {
    if (mAnnouncerRunning.exchange(true))
        return;
    mAnnouncerThread = std::thread(&ServerEmulator::AnnouncerLoop, this);
}

void ServerEmulator::StopAnnouncer() {
    {
        std::lock_guard lock(mAnnouncerMutex);
        if (!mAnnouncerRunning)
            return;
        mAnnouncerRunning = false;
    }
    mAnnouncerCv.notify_all();
    if (mAnnouncerThread.joinable())
        mAnnouncerThread.join();

    // Best-effort farewell so we drop off the master list promptly instead of
    // waiting for the master's stale-server sweep.
    if (mAnnouncedToMaster) {
        SendMasterPing("Goodbye");
        mAnnouncedToMaster = false;
    }
}

void ServerEmulator::AnnouncerLoop() {
    while (true) {
        {
            std::unique_lock lock(mAnnouncerMutex);
            mAnnouncerCv.wait_for(lock, std::chrono::seconds(kAnnounceIntervalSecs),
                [this] { return !mAnnouncerRunning.load(); });
            if (!mAnnouncerRunning)
                return;
        }

        Registry* reg = mCore->GetRegistry();
        if (reg == nullptr)
            continue;
        bool enabled = reg->GetKeyValue<bool>("emu.master.announce").value_or(false);
        std::string url = reg->GetKeyValue<std::string>("emu.auth.master").value_or("");
        if (!enabled || url.empty()) {
            // If announcing was just turned off while we were live, say Goodbye once.
            if (mAnnouncedToMaster) {
                SendMasterPing("Goodbye");
                mAnnouncedToMaster = false;
            }
            continue;
        }

        bool hasServers = !GetRunningGameServers().empty();
        if (hasServers) {
            SendMasterPing(mAnnouncedToMaster ? "Heartbeat" : "Hello");
            mAnnouncedToMaster = true;
        } else if (mAnnouncedToMaster) {
            SendMasterPing("Goodbye");
            mAnnouncedToMaster = false;
        }
    }
}

void ServerEmulator::SendMasterPing(const std::string &event) {
    Registry* reg = mCore->GetRegistry();
    if (reg == nullptr)
        return;
    std::string url = reg->GetKeyValue<std::string>("emu.auth.master").value_or("");
    if (url.empty())
        return;
    if (url.back() == '/')
        url.pop_back();

    uint16_t port = static_cast<uint16_t>(reg->GetKeyValue<int>("emu.https_port").value_or(53640));
    std::string name = reg->GetKeyValue<std::string>("emu.branding.title").value_or("noobWarrior Server Emulator");

    auto servers = GetRunningGameServers();

    json body;
    body["Event"] = event;
    body["Port"] = port;
    body["Name"] = name;
    body["Version"] = NOOBWARRIOR_VERSION;
    body["ServerCount"] = static_cast<int>(servers.size());
    // Surface the first server's placeId so the master can show a "Game" column.
    if (!servers.empty() && servers.front().PlaceId.has_value())
        body["PlaceId"] = servers.front().PlaceId.value();
    
    json serverArr = json::array();
    for (const auto &s : servers) {
        json sj;
        sj["Pid"] = s.Pid;
        sj["Version"] = s.Version;
        sj["PlaceId"] = s.PlaceId.has_value() ? json(s.PlaceId.value()) : json(nullptr);
        sj["Port"] = s.Port.has_value() ? json(s.Port.value()) : json(nullptr);
        serverArr.push_back(std::move(sj));
    }
    body["Servers"] = std::move(serverArr);

    cpr::Response res = cpr::Post(
        cpr::Url{url + "/v1/emu-ping"},
        cpr::Header{{"Content-Type", "application/json"}},
        cpr::Body{body.dump()},
        cpr::Timeout{std::chrono::milliseconds(5000)}
    );
    if (res.error.code != cpr::ErrorCode::OK)
        Out(mLogName, "Master announce ({}) to {} failed: {}", event, url, res.error.message);
    else if (res.status_code >= 400)
        Out(mLogName, "Master announce ({}) to {} got HTTP {}", event, url, static_cast<long>(res.status_code));
}
