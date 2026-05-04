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
// File: RunningGameServersHandler.cpp
// Started by: Hattozo
// Started on: 4/24/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/RunningGameServersHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/Log.h>

#include <nlohmann/json.hpp>

using namespace NoobWarrior;

RunningGameServersHandler::RunningGameServersHandler(ServerEmulator* emu) : mEmu(emu) {

}

void RunningGameServersHandler::OnRequest(evhttp_request *req, void *userdata) {
    Out("RunningGameServersHandler", "Yes.");
    nlohmann::json json = nlohmann::json::array();
    for (auto &gameServer : mEmu->GetGameServers()) {
        nlohmann::json obj = nlohmann::json::object();
        obj["Ip"] = gameServer.Ip;
        if (gameServer.Port.has_value()) obj["Port"] = gameServer.Port.value();
        if (gameServer.PlaceId.has_value()) obj["PlaceId"] = gameServer.PlaceId.value();
        obj["EngineType"] = EngineTypeAsString(gameServer.Engine.Type);
        obj["EngineVersion"] = gameServer.Engine.Version;
        json.push_back(obj);
    }

    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* reply = evbuffer_new();
    evbuffer_add_printf(reply, "%s", json.dump().c_str());
    evhttp_send_reply(req, 200, nullptr, reply);
    evbuffer_free(reply);
}
