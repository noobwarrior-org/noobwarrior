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
// File: ProcessPingHandler.cpp
// Started by: Hattozo
// Started on: 5/11/2026
// Description: Receives lifecycle events (Hello/Goodbye) from noobHook so the emulator
//              can track which Roblox processes are currently alive.
#include <NoobWarrior/HttpServer/Emulator/ProcessPingHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/Log.h>

#include <nlohmann/json.hpp>

using namespace NoobWarrior;

ProcessPingHandler::ProcessPingHandler(ServerEmulator* emu) : mEmu(emu) {

}

static EngineSide ParseSide(const std::string &s) {
    if (s == "Server") return EngineSide::Server;
    if (s == "Studio") return EngineSide::Studio;
    return EngineSide::Client;
}

void ProcessPingHandler::OnRequest(evhttp_request *req, void *userdata) {
    if (evhttp_request_get_command(req) != EVHTTP_REQ_POST) {
        evhttp_send_error(req, HTTP_BADMETHOD, "Request method needs to be POST");
        return;
    }

    evbuffer *buf = evhttp_request_get_input_buffer(req);
    size_t len = evbuffer_get_length(buf);
    std::string body(len, '\0');
    evbuffer_copyout(buf, body.data(), len);

    nlohmann::json json;
    try {
        json = nlohmann::json::parse(body);
    } catch (nlohmann::json::exception &ex) {
        Out("ProcessPingHandler", "Malformed ping body: {}", ex.what());
        evhttp_send_error(req, HTTP_BADREQUEST, "Malformed JSON");
        return;
    }

    std::string event = json.value("Event", "");
    int pid = json.value("Pid", 0);
    if (pid <= 0) {
        evhttp_send_error(req, HTTP_BADREQUEST, "Missing or invalid Pid");
        return;
    }

    if (event == "Goodbye") {
        mEmu->UnregisterInstance(pid);
        evhttp_send_reply(req, HTTP_OK, nullptr, nullptr);
        return;
    }

    if (event == "Heartbeat") {
        // Refresh LastSeen so the row doesn't get reaped
        if (mEmu->TouchInstance(pid)) {
            evhttp_send_reply(req, HTTP_OK, nullptr, nullptr);
            return;
        }
    } else if (event != "Hello") {
        evhttp_send_error(req, HTTP_BADREQUEST, "Unknown Event");
        return;
    }

    RunningInstance instance {};
    instance.Pid = pid;
    instance.Side = ParseSide(json.value("Side", "Client"));
    instance.Version = json.value("Version", "");
    if (json.contains("Port") && json["Port"].is_number_unsigned())
        instance.Port = static_cast<uint16_t>(json["Port"].get<unsigned>());
    if (json.contains("PlaceId") && json["PlaceId"].is_number_integer())
        instance.PlaceId = json["PlaceId"].get<int64_t>();

    // Peer IP comes from the connection, not the body, so a process can't lie about it.
    evhttp_connection *conn = evhttp_request_get_connection(req);
    if (conn != nullptr) {
        const char *peer = "";
        uint16_t peerPort = 0;
        evhttp_connection_get_peer(conn, &peer, &peerPort);
        if (peer) instance.Ip = peer;
    }

    mEmu->RegisterInstance(instance);
    evhttp_send_reply(req, HTTP_OK, nullptr, nullptr);
}
