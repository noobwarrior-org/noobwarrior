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
// File: PlaceLauncherHandler.cpp
// Started by: Hattozo
// Started on: 3/22/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/PlaceLauncherHandler.h>
#include <NoobWarrior/Log.h>

#include <nlohmann/json.hpp>

static constexpr const char* JSON = R"({"jobId":"Test","status":2,"joinScriptUrl":"http://www.roblox.com/Game/Join.ashx?placeid=1818&ip=localhost&port=53640&user=greg&id=1&membership=","authenticationUrl":"http://www.roblox.com/Login/Negotiate.ashx","authenticationTicket":"1","message":null})";

using namespace NoobWarrior;

PlaceLauncherHandler::PlaceLauncherHandler() {

}

void PlaceLauncherHandler::OnRequest(evhttp_request *req, void *userdata) {
    const char* uri = evhttp_request_get_uri(req);
    evhttp_connection* conn = evhttp_request_get_connection(req);

    const char* peer_address = "";
    uint16_t peer_port {};

    if (conn != NULL)
        evhttp_connection_get_peer(conn, &peer_address, &peer_port);
    Out("PlaceLauncherHandler", "{}:{} requested {}", peer_address, peer_port, uri);

    evkeyvalq headers;
    if (evhttp_parse_query(uri, &headers) != 0) {
        evhttp_send_error(req, 500, "Failed to parse URL parameters");
        return;
    }

    const char* ipStr = evhttp_find_header(&headers, "ip");
    const char* portStr = evhttp_find_header(&headers, "port");
    const char* localStr = evhttp_find_header(&headers, "local");

    nlohmann::json json = nlohmann::json::object();
    json["jobId"] = "Test";
    json["status"] = 2;
    json["joinScriptUrl"] = "http://www.roblox.com/Game/Join.ashx?ip=localhost&port=53640&local=";
    json["authenticationUrl"] = "http://www.roblox.com/Login/Negotiate.ashx";
    json["authenticationTicket"] = "1";
    json["message"] = nullptr;

    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* reply = evbuffer_new();
    evbuffer_add_printf(reply, json.dump().c_str());
    evhttp_send_reply(req, 200, nullptr, reply);
    evbuffer_free(reply);
}
