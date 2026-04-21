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
// File: ClientSettingsHandler.cpp
// Started by: Hattozo
// Started on: 11/16/2025
// Description: Returns a JSON object containing application settings (FFlags & DFFlags)
#include <NoobWarrior/HttpServer/Emulator/ClientSettingsHandler.h>
#include <NoobWarrior/Log.h>

#include "FFlagJson/PCDesktopClient.json.inc.cpp"

using namespace NoobWarrior;

ClientSettingsHandler::ClientSettingsHandler(ServerEmulator *server) {}

void ClientSettingsHandler::OnRequest(evhttp_request *req, void *userdata) {
    const char* uri = evhttp_request_get_uri(req);
    evhttp_connection* conn = evhttp_request_get_connection(req);

    const char* peer_address = "";
    uint16_t peer_port {};

    if (conn != NULL)
        evhttp_connection_get_peer(conn, &peer_address, &peer_port);
    Out("ClientSettingsHandler", "{}:{} requested client settings {}", peer_address, peer_port, uri);

    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* reply = evbuffer_new();
    evbuffer_add_printf(reply, "%s", PCDesktopClient_json);
    evhttp_send_reply(req, 200, nullptr, reply);
    evbuffer_free(reply);
}