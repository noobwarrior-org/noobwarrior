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

#include "ProcessPingHandler.h"
#include "CreateAccountHandler.h"
#include "LoginHandler.h"
#include "LogoutHandler.h"
#include "RunningGameServersHandler.h"
#include "ClientSettingsHandler.h"
#include "NegotiateHandler.h"
#include "PlaceLauncherHandler.h"
#include "JoinScriptJsonHandler.h"
#include "MySettingsJsonHandler.h"
#include "AuthenticatedUserHandler.h"
#include "CurrentUserHandler.h"
#include "RequestAuthHandler.h"
#include "StudioEditHandler.h"
#include "AssetHandler.h"
#include "AssetThumbnailJsonHandler.h"
#include "GameIconHandler.h"

#include <cstdint>
#include <filesystem>
#include <mutex>
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

    void SetCurrentEngine(const Engine &engine);
    std::optional<Engine> GetCurrentEngine();
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
    NegotiateHandler mNegotiateHandler;
    PlaceLauncherHandler mPlaceLauncherHandler;
    JoinScriptJsonHandler mJoinScriptJsonHandler;
    MySettingsJsonHandler mMySettingsJsonHandler;
    AuthenticatedUserHandler mAuthenticatedUserHandler;
    CurrentUserHandler mCurrentUserHandler;
    RequestAuthHandler mRequestAuthHandler;
    StudioEditHandler mStudioEditHandler;
    GameIconHandler mGameIconHandler;

    void SweepStaleInstancesLocked();

    static constexpr int kStaleInstanceThresholdSecs = 30;

    mutable std::mutex mInstancesMutex;
    std::vector<RunningInstance> mInstances;
};
}
