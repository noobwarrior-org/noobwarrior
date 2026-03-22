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

ServerEmulator::ServerEmulator(Core *core) : HttpServer(core, "ServerEmulator") {

}

ServerEmulator::~ServerEmulator() {}

int ServerEmulator::Start(uint16_t port) {
    int res = HttpServer::Start(port);
    if (!res) goto finish;

    mAssetHandler = std::make_unique<AssetHandler>(this, mCore->GetEmuDbManager());
    mClientSettingsHandler = std::make_unique<ClientSettingsHandler>(this);
    mStudioEditHandler = std::make_unique<StudioEditHandler>();

    SetRequestHandler("/Asset", mAssetHandler.get());
    SetRequestHandler("/asset", mAssetHandler.get());
    SetRequestHandler("/v1/asset", mAssetHandler.get());

    SetRequestHandler("/v1/settings/application", mClientSettingsHandler.get());

    SetRequestHandler("/game/edit.ashx", mStudioEditHandler.get());
finish:
    return res;
}

int ServerEmulator::Stop() {
    return HttpServer::Stop();
}
