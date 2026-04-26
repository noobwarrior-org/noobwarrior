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
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/HttpServer/Emulator/ClientSettingsHandler.h>
#include <NoobWarrior/NoobWarrior.h>

#include "NoobWarrior/HttpServer/Emulator/RequestAuthHandler.h"

using namespace NoobWarrior;
using json = nlohmann::json;

ServerEmulator::ServerEmulator(Core *core) : HttpServer(core, "ServerEmulator"),
    mRunningGameServersHandler(this),
    mAssetHandler(this, mCore->GetEmuDbManager()),
    mAssetThumbnailJsonHandler(this, mCore->GetEmuDbManager()),
    mClientSettingsHandler(this),
    mNegotiateHandler(),
    mPlaceLauncherHandler(),
    mJoinScriptJsonHandler(),
    mMySettingsJsonHandler(),
    mAuthenticatedUserHandler(),
    mCurrentUserHandler(),
    mRequestAuthHandler(),
    mStudioEditHandler(),
    mGameIconHandler(this, mCore->GetEmuDbManager())
{
}

ServerEmulator::~ServerEmulator() {}

int ServerEmulator::Start(uint16_t port) {
    int res = HttpServer::Start(port);
    if (!res) goto finish;

    SetRequestHandler("/v1/running-game-servers", &mRunningGameServersHandler);

    SetRequestHandler("/Asset", &mAssetHandler);
    SetRequestHandler("/asset", &mAssetHandler);
    SetRequestHandler("/asset/", &mAssetHandler);
    SetRequestHandler("/v1/asset", &mAssetHandler);
    SetRequestHandler("/v1/asset/", &mAssetHandler);

    SetRequestHandler("/asset-thumbnail/json", &mAssetThumbnailJsonHandler);

    SetRequestHandler("/v1/settings/application", &mClientSettingsHandler);

    SetRequestHandler("/Login/Negotiate.ashx", &mNegotiateHandler);
    SetRequestHandler("/login/negotiate.ashx", &mNegotiateHandler);

    SetRequestHandler("/Game/PlaceLauncher.ashx", &mPlaceLauncherHandler);
    SetRequestHandler("/game/placelauncher.ashx", &mPlaceLauncherHandler);

    SetRequestHandler("/Game/Join.ashx", &mJoinScriptJsonHandler);
    SetRequestHandler("/game/join.ashx", &mJoinScriptJsonHandler);

    SetRequestHandler("/my/settings/json", &mMySettingsJsonHandler);

    SetRequestHandler("/v1/users/authenticated", &mAuthenticatedUserHandler);

    SetRequestHandler("/game/GetCurrentUser.ashx", &mCurrentUserHandler);

    SetRequestHandler("/Login/RequestAuth.ashx", &mRequestAuthHandler);
    SetRequestHandler("/login/RequestAuth.ashx", &mRequestAuthHandler);

    SetRequestHandler("/Game/Edit.ashx", &mStudioEditHandler);
    SetRequestHandler("/game/edit.ashx", &mStudioEditHandler);

    SetRequestHandler("/Thumbs/GameIcon.ashx", &mGameIconHandler);
    SetRequestHandler("/Thumbs/gameicon.ashx", &mGameIconHandler);
finish:
    return res;
}

int ServerEmulator::Stop() {
    return HttpServer::Stop();
}

void ServerEmulator::SetMode(Mode mode) {
}

ServerEmulator::Mode ServerEmulator::GetMode() {
}

void ServerEmulator::AddTemporaryProxy(const std::string &ip, uint16_t port) {
    mTemporaryProxies.emplace_back( ip, port );
}

void ServerEmulator::RemoveTemporaryProxy(const std::string &ip, uint16_t port) {
    for (int i = 0; i < mTemporaryProxies.size(); i++) {
        std::pair<std::string, uint16_t> ipAndPort = mTemporaryProxies.at(i);
        if (ipAndPort.first == ip && ipAndPort.second == port) {
            mTemporaryProxies.erase(mTemporaryProxies.begin() + i);
        }
    }
}

void ServerEmulator::AddGameServer(const EngineStartParameters &params) {
    mGameServers.emplace_back(params);
}

void ServerEmulator::RemoveGameServer(const std::string &ip, uint16_t port) {
    for (int i = 0; i < mGameServers.size(); i++) {
        EngineStartParameters gameServer = mGameServers.at(i);
        if (gameServer.Ip == ip && gameServer.Port == port) {
            mGameServers.erase(mGameServers.begin() + i);
        }
    }
}

std::vector<EngineStartParameters> &ServerEmulator::GetGameServers() {
    return mGameServers;
}

void ServerEmulator::SetCurrentEngine(const Engine &engine) {
    mCurrentEngine = engine;
}

std::optional<Engine> ServerEmulator::GetCurrentEngine() {
    return mCurrentEngine;
}
