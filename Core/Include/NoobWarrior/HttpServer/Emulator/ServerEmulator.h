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
// File: ServerEmulator.h
// Started by: Hattozo
// Started on: 9/2/2025
// Description:
#pragma once
#include <NoobWarrior/HttpServer/Base/HttpServer.h>
#include <NoobWarrior/Engine.h>
#include <NoobWarrior/Url.h>
#include <NoobWarrior/EmuDb/AssetEnricher.h>

#include "ProcessPingHandler.h"
#include "CreateAccountHandler.h"
#include "LoginHandler.h"
#include "LogoutHandler.h"
#include "RunningGameServersHandler.h"
#include "ClientSettingsHandler.h"
#include "ClientSettingsV2StudioHandler.h"
#include "NegotiateHandler.h"
#include "PlaceLauncherHandler.h"
#include "JoinScriptJsonHandler.h"
#include "GameJoinHandler.h"
#include "MySettingsJsonHandler.h"
#include "AuthenticatedUserHandler.h"
#include "SessionCheckHandler.h"
#include "AvatarSetHandler.h"
#include "AvatarCatalogHandler.h"
#include "AvatarThumbnailHandler.h"
#include "CurrentUserHandler.h"
#include "RequestAuthHandler.h"
#include "StudioEditHandler.h"
#include "AssetHandler.h"
#include "AssetBatchHandler.h"
#include "AssetThumbnailJsonHandler.h"
#include "GameIconHandler.h"
#include "UniversalAppConfigStudioHandler.h"
#include "AppBehaviorsHandler.h"
#include "AvatarHandler.h"
#include "AvatarRulesHandler.h"
#include "AppLaunchInfoHandler.h"
#include "RolesHandler.h"
#include "LocalesHandler.h"
#include "GamesHandler.h"
#include "UserChannelHandler.h"
#include "UserProfilesHandler.h"
#include "PlaceDetailsHandler.h"
#include "PlaceUniverseHandler.h"
#include "ToolboxServiceHandler.h"
#include "IdeToolboxHandler.h"
#include "DevelopHandler.h"
#include "DataUploadHandler.h"
#include "IdePublishHandler.h"
#include "DataStorePersistenceHandler.h"
#include "ThumbnailHandler.h"
#include "AssetPermissionsHandler.h"
#include "OmniRecHandler.h"
#include "GamesListHandler.h"
#include "GamesSortsHandler.h"
#include "ClientSettingsV2DesktopHandler.h"
#include "OAuthDiscoveryHandler.h"
#include "OAuthTokenHandler.h"
#include "OAuthUserInfoHandler.h"
#include "StudioLoginHandler.h"
#include "ClientVersionStudioHandler.h"
#include "StudioPbeHandler.h"
#include "GuacBundlesStudioHandler.h"
#include "UserModerationHandler.h"
#include "VoiceChatHandler.h"
#include "SignalRCoreHandler.h"
#include "OAuthAuthorizeHandler.h"
#include "StudioOpenPlaceHandler.h"
#include "AuthTicketRedeemHandler.h"
#include "AvatarFetchHandler.h"
#include "AvatarOverrideHandler.h"
#include "AuthInfoHandler.h"
#include "AuthUtil.h"
#include "EmulatorProxy.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace NoobWarrior {
class Core;
struct RunningInstance {
    int Pid {0};
    EngineSide Side {};
    std::string Version {};
    std::string Hash {};  // engine folder ("version-<hash>"); the client-version upload id
    std::string Ip {};
    std::optional<uint16_t> Port {std::nullopt};
    std::optional<int64_t> PlaceId {std::nullopt};
    // Windows FILETIME creation value captured for loopback processes. It distinguishes the
    // registered process from a later process that happens to reuse the same PID.
    std::optional<uint64_t> ProcessStartToken {std::nullopt};
    time_t FirstSeen {0};
    time_t LastSeen {0};
};

class ServerEmulator : public HttpServer {
public:
    static constexpr const char *kIdentityHeader = "X-NoobWarrior-Emulator-Id";
    static constexpr const char *kProxyChainHeader = "X-NoobWarrior-Proxy-Chain";

    enum class Mode {
        Local,
        Online
    };

    ServerEmulator(Core *core);
    ~ServerEmulator();

    int Start(uint16_t port) override;
    int Stop() override;

    void SetMode(Mode mode);
    Mode GetMode();

    /* Lifecycle tracking driven by /v1/process-ping.
     * Hello/Goodbye are the primary signals, but processes can die without firing Goodbye
     * (TerminateProcess, crashes, network blips). Heartbeats from noobHook bump LastSeen;
     * SweepStaleInstances drops rows that haven't been heard from in a while. */
    void RegisterInstance(const RunningInstance &instance);
    void UnregisterInstance(int pid);
    bool TouchInstance(int pid); // returns false if the PID isn't tracked
    std::vector<RunningInstance> GetRunningInstances() const;
    std::vector<RunningInstance> GetRunningGameServers() const; // Side == Server subset

    // Random for each Core run. The server-list handshake uses this to recognize the same emulator
    // reached through its public address, while the proxy chain uses it to terminate routing loops.
    const std::string &GetInstanceId() const;

    // Version/hash of the most recently launched Studio engine.
    // This function is called so ClientVersionStudioHandler always has reliable results on what Studio version is running.
    void SetLaunchedStudioVersion(const std::string &version, const std::string &hash);
    std::pair<std::string, std::string> GetLaunchedStudioVersion() const;
    
    std::string ResolveAdvertisedAddress(const std::string &localAddr,
                                         bool waitForDetection = false);

    void PushProxyLayer(const std::string &host, uint16_t port, const std::string &sessionToken = "");
    bool PopProxyLayer();
    void RemoveProxyLayer(const std::string &host, uint16_t port);
    void ClearProxyLayers();
    std::vector<std::pair<std::string, uint16_t>> GetProxyLayers() const;

    /* The place we joined on a remote host, recorded when that join's proxy layer is pushed and
     * cleared with the layers. A joining client never reports a place id of its own (noobhook only
     * reads one on the server side), so this is our only record of what the user is playing. */
    void SetJoinedPlaceId(std::optional<int64_t> placeId);
    std::optional<int64_t> GetJoinedPlaceId() const;
    
    bool TryProxyRequest(evhttp_request *req,
                         std::function<void(evhttp_request *)> localFallback = {},
                         EmulatorProxy::ResponseTransform responseTransform = {});

    /* Avatar appearance federation. A joining client only knows its own appearance (it lives in that
     * client's local registry), so on join it POSTs that appearance to the host it's joining, keyed
     * by its user id. The host caches it here; AvatarFetchHandler then serves the cached appearance
     * when its game server asks for that user id, so the joining player's character is built with
     * their own look instead of the host's. avatarFetchJson is the /v1.1/avatar-fetch response body. */
    void SetAvatarOverride(int64_t userId, const std::string &avatarFetchJson);
    std::optional<std::string> GetAvatarOverride(int64_t userId) const;
    void ClearAvatarOverrides();

    /* A federated joiner's avatar lives on their home master, not in our DB. On join we record where to
     * fetch it (keyed by their OnlineUserId); GetFederatedAvatar pulls it once and caches it, so
     * AvatarFetchHandler serves the home-master look instead of the local default. baseUrl is the home
     * master's base (for pulling the joiner's worn assets on demand, see TryFetchFederatedAsset). */
    void SetFederatedAvatarSource(int64_t userId, const std::string &sourceUrl, const std::string &baseUrl);
    std::optional<std::string> GetFederatedAvatar(int64_t userId);

    /* On-demand asset proxy: a federated player's worn items (and the mesh/texture deps the engine fetches
     * for them) live on their home master. When an asset id misses locally, this tries every joined
     * federated player's home master's /fed/v1/asset, caches a hit in the temporary db (so the engine's
     * next fetch resolves locally), and returns the bytes. false if no federated origin has it. */
    bool TryFetchFederatedAsset(int64_t id, std::vector<unsigned char> *dataOut, std::string *hashOut);

    /* Records that OnlineUserId is held by handle this run. Returns false if that id is already bound to
     * a different handle (an identity collision/spoof), so the join can be refused. */
    bool BindFederatedHandle(int64_t userId, const std::string &handle);

    // File name of the database holding the place currently open for editing (set by
    // StudioOpenPlaceHandler). Lets descendant-asset publishing land in the same .nwdb as the game.
    void SetActiveEditDbFile(const std::string &dbFileName);
    std::string GetActiveEditDbFile() const;

    // Id of that same place (also set by StudioOpenPlaceHandler).
    void SetActiveEditPlaceId(int64_t placeId);
    std::optional<int64_t> GetActiveEditPlaceId() const;

    /* The place noobWarrior currently has loaded, i.e. what a request served right now belongs to:
     * the running game server's place (from the gameserver.json it was launched with), else any other
     * running instance that reported one (a Studio team-test host gets it from the injector's
     * --placeid), else the place Studio has open for editing. nullopt when nothing is loaded. */
    std::optional<int64_t> GetCurrentPlaceId() const;

    // Identity from the most recent launch-ticket redemption. A launched client redeems its -t ticket
    // to authenticate, then joins; the local join reads this instead of the client's persistence-prone
    // .LOGINSESSION cookie. Assumes one local launch at a time.
    void SetCurrentLaunchUser(const AuthUtil::SessionUser &user);
    std::optional<AuthUtil::SessionUser> GetCurrentLaunchUser() const;

    // The identity joining this server: the just-launched local client's identity (from its redeemed
    // launch ticket), else the joiner's forwarded .LOGINSESSION cookie (a remote proxied join). The
    // single source of truth for who is joining; nullopt when nobody is identified.
    std::optional<AuthUtil::SessionUser> ResolveJoiningUser(evhttp_request *req);

    // Slave mode: verifies a "fedvoucher." credential with our configured master (emu.auth.master),
    // which vouches for the federated identity over federation. nullopt if invalid/refused. Public so a
    // loopback join (where the voucher rides the -t launch ticket, not a cookie) can redeem it too.
    std::optional<AuthUtil::SessionUser> ResolveFederatedVoucher(const std::string &cookieValue);

    // Background worker that fills in metadata + thumbnails for assets captured by assetGrabMode.
    AssetEnricher* GetAssetEnricher();
private:
    Mode mMode;
    std::optional<Engine> mCurrentEngine;

    //////////////// Handlers ////////////////
    ProcessPingHandler mProcessPingHandler;
    CreateAccountHandler mCreateAccountHandler;
    LoginHandler mLoginHandler;
    LogoutHandler mLogoutHandler;
    RunningGameServersHandler mRunningGameServersHandler;
    AssetHandler mAssetHandler;
    AssetBatchHandler mAssetBatchHandler;
    AssetThumbnailJsonHandler mAssetThumbnailJsonHandler;
    ClientSettingsHandler mClientSettingsHandler;
    ClientSettingsV2StudioHandler mClientSettingsV2StudioHandler;
    ClientSettingsV2DesktopHandler mClientSettingsV2DesktopHandler;
    NegotiateHandler mNegotiateHandler;
    PlaceLauncherHandler mPlaceLauncherHandler;
    JoinScriptJsonHandler mJoinScriptJsonHandler;
    GameJoinHandler mGameJoinHandler;
    AuthTicketRedeemHandler mAuthTicketRedeemHandler;
    AppBehaviorsHandler mAppBehaviorsHandler;
    AvatarRulesHandler mAvatarRulesHandler;
    AvatarHandler mAvatarHandler;
    AppLaunchInfoHandler mAppLaunchInfoHandler;
    RolesHandler mRolesHandler;
    LocalesHandler mLocalesHandler;
    GamesHandler mGamesHandler;
    UserChannelHandler mUserChannelHandler;
    UserProfilesHandler mUserProfilesHandler;
    PlaceDetailsHandler mPlaceDetailsHandler;
    PlaceUniverseHandler mPlaceUniverseHandler;
    ToolboxServiceHandler mToolboxServiceHandler;
    IdeToolboxHandler mIdeToolboxHandler;
    DevelopHandler mDevelopHandler;
    DataUploadHandler mDataUploadHandler;
    IdePublishHandler mIdePublishHandler;
    ThumbnailHandler mThumbnailHandler;
    AssetPermissionsHandler mAssetPermissionsHandler;
    OmniRecHandler mOmniRecHandler;
    GamesSortsHandler mGamesSortsHandler;
    AvatarFetchHandler mAvatarFetchHandler;
    AvatarOverrideHandler mAvatarOverrideHandler;
    AuthInfoHandler mAuthInfoHandler;
    GamesListHandler mGamesListHandler;
    UniversalAppConfigStudioHandler mUniversalAppConfigStudioHandler;
    MySettingsJsonHandler mMySettingsJsonHandler;
    AuthenticatedUserHandler mAuthenticatedUserHandler;
    SessionCheckHandler mSessionCheckHandler;
    AvatarSetHandler mAvatarSetHandler;
    AvatarCatalogHandler mAvatarCatalogHandler;
    AvatarThumbnailHandler mAvatarThumbnailHandler;
    CurrentUserHandler mCurrentUserHandler;
    RequestAuthHandler mRequestAuthHandler;
    StudioEditHandler mStudioEditHandler;
    GameIconHandler mGameIconHandler;
    OAuthDiscoveryHandler mOAuthDiscoveryHandler;
    OAuthAuthorizeHandler mOAuthAuthorizeHandler;
    OAuthTokenHandler mOAuthTokenHandler;
    OAuthUserInfoHandler mOAuthUserInfoHandler;
    StudioLoginHandler mStudioLoginHandler;
    StudioOpenPlaceHandler mStudioOpenPlaceHandler;
    DataStorePersistenceHandler mDataStorePersistenceHandler;

    // Studio 0.729 boot endpoints. client-version reports the running version so Studio
    // never tries to update; /studio/pbe returns an empty list. Own classes (rather than
    // canned StaticJsonHandler) so they can be made data-driven later.
    ClientVersionStudioHandler mClientVersionStudioHandler;
    StudioPbeHandler mStudioPbeHandler;
    GuacBundlesStudioHandler mGuacBundlesStudioHandler;
    UserModerationHandler mUserModerationHandler;
    VoiceChatHandler mVoiceChatHandler;
    SignalRCoreHandler mSignalRCoreHandler;

    std::string mInstanceId;

    // Layered reverse proxy to the remote emulator(s) the local client is currently joined to.
    EmulatorProxy mEmulatorProxy;

    void SetupHandlers() override;
    void SweepStaleInstancesLocked();

    static constexpr int kStaleInstanceThresholdSecs = 30;

    mutable std::mutex mInstancesMutex;
    std::vector<RunningInstance> mInstances;

    mutable std::mutex mLaunchedStudioMutex;
    std::string mLaunchedStudioVersion {};
    std::string mLaunchedStudioHash {};

    // Avatar appearances federated from joining clients, keyed by their user id (see the public
    // Set/Get/ClearAvatarOverride methods). Each value is a /v1.1/avatar-fetch response body.
    mutable std::mutex mAvatarOverridesMutex;
    std::map<int64_t, std::string> mAvatarOverrides;

    // Federated joiners' avatars, keyed by OnlineUserId: where to fetch (SourceUrl) and, once pulled,
    // the cached /v1.1/avatar-fetch body (see Set/GetFederatedAvatar).
    struct FederatedAvatar { std::string SourceUrl; std::string BaseUrl; std::string CachedJson; };
    mutable std::mutex mFederatedAvatarsMutex;
    std::map<int64_t, FederatedAvatar> mFederatedAvatars;

    // Which handle first claimed each federated OnlineUserId this run. An OnlineUserId is a hash of the
    // handle, so the same id under a different handle means a collision/spoof attempt (see BindFederatedHandle).
    mutable std::mutex mFederatedHandlesMutex;
    std::map<int64_t, std::string> mFederatedHandles;

    // Database file name and id of the place currently open for editing (see Set/GetActiveEditDbFile).
    mutable std::mutex mActiveEditDbMutex;
    std::string mActiveEditDbFile;
    std::optional<int64_t> mActiveEditPlaceId;

    // Place joined on a remote host (see Set/GetJoinedPlaceId).
    mutable std::mutex mJoinedPlaceMutex;
    std::optional<int64_t> mJoinedPlaceId;

    mutable std::mutex mCurrentLaunchUserMutex;
    std::optional<AuthUtil::SessionUser> mCurrentLaunchUser;

    AssetEnricher mAssetEnricher;

    /* Master-list announcer. When emu.master.announce is enabled, a background
     * thread periodically POSTs a Hello/Heartbeat to emu.auth.master/v1/emu-ping
     * for as long as there is at least one running game server (Side == Server), and
     * a Goodbye once there are none (or on shutdown). This is the outbound counterpart
     * to ProcessPingHandler's inbound process-pings. */
    void StartAnnouncer();
    void StopAnnouncer();
    void AnnouncerLoop();
    void SendMasterPing(const std::string &event);

    static constexpr int kAnnounceIntervalSecs = 10;

    std::thread mAnnouncerThread;
    std::atomic<bool> mAnnouncerRunning {false};
    std::condition_variable mAnnouncerCv;
    std::mutex mAnnouncerMutex;
    bool mAnnouncedToMaster {false};
};
}
