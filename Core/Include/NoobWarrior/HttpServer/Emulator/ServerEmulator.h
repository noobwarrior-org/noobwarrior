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
#include <vector>
#include <queue>
#include <utility>

namespace NoobWarrior {
class Core;
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

    void SetCurrentlyPlayingServer();

    void SetCurrentEngine(const Engine &engine);
    std::optional<Engine> GetCurrentEngine();
private:
    Mode mMode;
    std::optional<Engine> mCurrentEngine;

    //////////////// Handlers ////////////////
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

    std::priority_queue<std::pair<uint16_t, std::string>> TemporaryProxies;
};
}
