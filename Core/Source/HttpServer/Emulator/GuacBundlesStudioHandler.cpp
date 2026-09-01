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
// File: GuacBundlesStudioHandler.cpp
// Started by: Hattozo
// Started on: 8/8/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/GuacBundlesStudioHandler.h>
#include <NoobWarrior/NoobWarrior.h>

#include <chrono>
#include <string>

using namespace NoobWarrior;

GuacBundlesStudioHandler::GuacBundlesStudioHandler() {}

void GuacBundlesStudioHandler::OnRequest(evhttp_request *req, void *userdata) {
    const char* uri = evhttp_request_get_uri(req);
    evhttp_connection* conn = evhttp_request_get_connection(req);

    const char* peer_address = "";
    uint16_t peer_port {};
    if (conn != NULL)
        evhttp_connection_get_peer(conn, &peer_address, &peer_port);
    mCore->Out("GuacBundlesStudioHandler", "{}:{} requested {}", peer_address, peer_port, uri);

    std::string path = uri ? uri : "";
    if (const size_t query = path.find('?'); query != std::string::npos)
        path.resize(query);

    std::string json;
    if (path == "/guac-v2/v1/bundles/app-policy") {
        // GUAC v2 bundle endpoints return the policy dictionary directly. The Player supplies
        // built-in defaults for omitted keys, so an empty local policy is sufficient.
        json = "{}";
    } else if (path == "/guac-v2/v1/bundles/intl-auth-compliance") {
        json = "{\"disableSignupCheckbox\":false}";
    } else {
        // Preserve the older Studio response that this handler was originally written for.
        long long nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        json =
            "{\"data\":{\"applicationSettings\":{}},\"hash\":\"noobwarrior-guac\",\"timestamp\":"
            + std::to_string(nowMs) + "}";
    }

    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* reply = evbuffer_new();
    evbuffer_add_printf(reply, "%s", json.c_str());
    evhttp_send_reply(req, 200, nullptr, reply);
    evbuffer_free(reply);
}
