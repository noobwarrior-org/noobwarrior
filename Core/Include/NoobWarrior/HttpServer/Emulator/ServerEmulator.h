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
#include "ClientSettingsV2Handler.h"
#include "NegotiateHandler.h"
#include "PlaceLauncherHandler.h"
#include "JoinScriptJsonHandler.h"
#include "GameJoinHandler.h"
#include "MySettingsJsonHandler.h"
#include "AuthenticatedUserHandler.h"
#include "CurrentUserHandler.h"
#include "RequestAuthHandler.h"
#include "StudioEditHandler.h"
#include "AssetHandler.h"
#include "AssetThumbnailJsonHandler.h"
#include "GameIconHandler.h"
#include "UniversalAppConfigStudioHandler.h"
#include "StaticJsonHandler.h"
#include "OAuthAuthorizeHandler.h"
#include "StudioOpenPlaceHandler.h"
#include "AuthTicketRedeemHandler.h"
#include "AvatarFetchHandler.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <thread>
#include <vector>

namespace NoobWarrior {
class Core;
struct RunningInstance {
    int Pid {0};
    EngineSide Side {};
    std::string Version {};
    std::string Ip {};
    std::optional<uint16_t> Port {std::nullopt};
    std::optional<int64_t> PlaceId {std::nullopt};
    time_t FirstSeen {0};
    time_t LastSeen {0};
};

class ServerEmulator : public HttpServer {
public:
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
    AssetThumbnailJsonHandler mAssetThumbnailJsonHandler;
    ClientSettingsHandler mClientSettingsHandler;
    ClientSettingsV2Handler mClientSettingsV2Handler;
    StaticJsonHandler mClientSettingsV2DesktopHandler;
    NegotiateHandler mNegotiateHandler;
    PlaceLauncherHandler mPlaceLauncherHandler;
    JoinScriptJsonHandler mJoinScriptJsonHandler;
    GameJoinHandler mGameJoinHandler;
    AuthTicketRedeemHandler mAuthTicketRedeemHandler;
    StaticJsonHandler mAppBehaviorsHandler;
    StaticJsonHandler mAvatarRulesHandler;
    StaticJsonHandler mAvatarHandler;
    StaticJsonHandler mAppLaunchInfoHandler;
    StaticJsonHandler mRolesHandler;
    StaticJsonHandler mLocalesHandler;
    StaticJsonHandler mGamesHandler;
    StaticJsonHandler mUserChannelHandler;
    StaticJsonHandler mPlaceDetailsHandler;
    StaticJsonHandler mOmniRecHandler;
    StaticJsonHandler mGamesSortsHandler;
    AvatarFetchHandler mAvatarFetchHandler;
    StaticJsonHandler mGamesListHandler;
    UniversalAppConfigStudioHandler mUniversalAppConfigStudioHandler;
    MySettingsJsonHandler mMySettingsJsonHandler;
    AuthenticatedUserHandler mAuthenticatedUserHandler;
    CurrentUserHandler mCurrentUserHandler;
    RequestAuthHandler mRequestAuthHandler;
    StudioEditHandler mStudioEditHandler;
    GameIconHandler mGameIconHandler;
    StaticJsonHandler mOAuthDiscoveryHandler;
    OAuthAuthorizeHandler mOAuthAuthorizeHandler;
    StaticJsonHandler mOAuthTokenHandler;
    StaticJsonHandler mOAuthUserinfoHandler;
    StaticJsonHandler mStudioLoginHandler;
    StudioOpenPlaceHandler mStudioOpenPlaceHandler;

    void SetupHandlers() override;
    void SweepStaleInstancesLocked();

    static constexpr int kStaleInstanceThresholdSecs = 30;

    mutable std::mutex mInstancesMutex;
    std::vector<RunningInstance> mInstances;

    AssetEnricher mAssetEnricher;

    /* Master-list announcer. When emu.master_list.announce is enabled, a background
     * thread periodically POSTs a Hello/Heartbeat to emu.master_list.url/v1/emu-ping
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
