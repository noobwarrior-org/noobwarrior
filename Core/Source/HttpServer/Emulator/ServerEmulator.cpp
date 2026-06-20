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
    mPlaceLauncherHandler(),
    mJoinScriptJsonHandler(),
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
    mThumbnailHandler(mCore->GetEmuDbManager()),
    mOmniRecHandler(),
    mGamesSortsHandler(),
    mGamesListHandler(),
    mAvatarFetchHandler(this),
    mAvatarOverrideHandler(this),
    mUniversalAppConfigStudioHandler(),
    mMySettingsJsonHandler(),
    mAuthenticatedUserHandler(),
    mCurrentUserHandler(),
    mRequestAuthHandler(),
    mStudioEditHandler(),
    mGameIconHandler(this, mCore->GetEmuDbManager()),
    mOAuthDiscoveryHandler(),
    mOAuthAuthorizeHandler(),
    mOAuthTokenHandler(),
    mOAuthUserinfoHandler(),
    mStudioLoginHandler(),
    mStudioOpenPlaceHandler(this),
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

    SetRequestHandler("/oauth/.well-known/openid-configuration", &mOAuthDiscoveryHandler);
    SetRequestHandler("/oauth/v1/authorize", &mOAuthAuthorizeHandler);
    SetRequestHandler("/oauth/v1/token", &mOAuthTokenHandler);
    SetRequestHandler("/oauth/v1/userinfo", &mOAuthUserinfoHandler);
    SetRequestHandler("/studio-login/v1/login", &mStudioLoginHandler);

    SetRequestHandler("/studio-open-place/v1/openplace", &mStudioOpenPlaceHandler);
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

void ServerEmulator::PushProxyLayer(const std::string &host, uint16_t port) {
    mEmulatorProxy.PushLayer(host, port);
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
    return mEmulatorProxy.GetLayers();
}

bool ServerEmulator::TryProxyRequest(evhttp_request *req, std::function<void(evhttp_request *)> localFallback,
                                     EmulatorProxy::ResponseTransform responseTransform) {
    return mEmulatorProxy.TryProxy(req, std::move(localFallback), std::move(responseTransform));
}

void ServerEmulator::SetAvatarOverride(int64_t userId, const std::string &avatarFetchJson) {
    std::lock_guard lock(mAvatarOverridesMutex);
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
        bool enabled = reg->GetKeyValue<bool>("emu.master_list.announce").value_or(false);
        std::string url = reg->GetKeyValue<std::string>("emu.master_list.url").value_or("");
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
    std::string url = reg->GetKeyValue<std::string>("emu.master_list.url").value_or("");
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
