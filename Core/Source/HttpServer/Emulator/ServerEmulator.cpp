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

using namespace NoobWarrior;
using json = nlohmann::json;

ServerEmulator::ServerEmulator(Core *core) : HttpServer(core, "ServerEmulator"),
    mAssetHandler(this, mCore->GetEmuDbManager()),
    mClientSettingsHandler(this),
    mStudioEditHandler()
{

}

ServerEmulator::~ServerEmulator() {}

int ServerEmulator::Start(uint16_t port) {
    int res = HttpServer::Start(port);
    if (!res) goto finish;

    SetRequestHandler("/Asset", &mAssetHandler);
    SetRequestHandler("/asset", &mAssetHandler);
    SetRequestHandler("/asset/", &mAssetHandler);
    SetRequestHandler("/v1/asset", &mAssetHandler);
    SetRequestHandler("/v1/asset/", &mAssetHandler);

    SetRequestHandler("/v1/settings/application", &mClientSettingsHandler);

    SetRequestHandler("/Login/Negotiate.ashx", &mAssetHandler);
    SetRequestHandler("/login/negotiate.ashx", &mAssetHandler);

    SetRequestHandler("/Game/PlaceLauncher.ashx", &mAssetHandler);
    SetRequestHandler("/game/placelauncher.ashx", &mAssetHandler);

    SetRequestHandler("/Game/Join.ashx", &mAssetHandler);
    SetRequestHandler("/game/join.ashx", &mAssetHandler);

    SetRequestHandler("/Game/Edit.ashx", &mStudioEditHandler);
    SetRequestHandler("/game/edit.ashx", &mStudioEditHandler);
finish:
    return res;
}

int ServerEmulator::Stop() {
    return HttpServer::Stop();
}
