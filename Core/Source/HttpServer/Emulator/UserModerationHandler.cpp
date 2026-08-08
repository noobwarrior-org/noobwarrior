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
// File: UserModerationHandler.cpp
// Started by: Hattozo
// Started on: 8/8/2026
// Description: what roblox studio requests to see if you are banned or not.
// obviously you can't get banned from noobWarrior, so return false always.
#include <NoobWarrior/HttpServer/Emulator/UserModerationHandler.h>
#include <NoobWarrior/Log.h>

using namespace NoobWarrior;

UserModerationHandler::UserModerationHandler() {}

void UserModerationHandler::OnRequest(evhttp_request *req, void *userdata) {
    const char* uri = evhttp_request_get_uri(req);
    evhttp_connection* conn = evhttp_request_get_connection(req);

    const char* peer_address = "";
    uint16_t peer_port {};
    if (conn != NULL)
        evhttp_connection_get_peer(conn, &peer_address, &peer_port);
    Out("UserModerationHandler", "{}:{} requested {}", peer_address, peer_port, uri);

    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* reply = evbuffer_new();
    evbuffer_add_printf(reply, "%s", "{\"notApproved\":false}");
    evhttp_send_reply(req, 200, nullptr, reply);
    evbuffer_free(reply);
}
