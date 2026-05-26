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
// File: StudioOpenPlaceHandler.cpp
// Started by: Hattozo
// Started on: 5/26/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/StudioOpenPlaceHandler.h>
#include <NoobWarrior/Log.h>

static constexpr const char* JSON = R"***({"universe":{"Id":1818,"RootPlaceId":1,"Name":"Hello","IsArchived":false,"CreatorType":"User","CreatorTargetId":1,"PrivacyType":"Public","Created":"2013-11-01T08:47:14.07+00:00","Updated":"2023-05-02T22:03:01.107+00:00"},"teamCreateEnabled":false,"place":{"Creator":{"CreatorType":"User","CreatorTargetId":1}}})***";

using namespace NoobWarrior;

StudioOpenPlaceHandler::StudioOpenPlaceHandler(ServerEmulator *server) {}

void StudioOpenPlaceHandler::OnRequest(evhttp_request *req, void *userdata) {
    const char* uri = evhttp_request_get_uri(req);
    evhttp_connection* conn = evhttp_request_get_connection(req);

    const char* peer_address = "";
    uint16_t peer_port {};

    if (conn != NULL)
        evhttp_connection_get_peer(conn, &peer_address, &peer_port);
    Out("StudioOpenPlaceHandler", "{}:{} requested {}", peer_address, peer_port, uri);

    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* reply = evbuffer_new();
    evbuffer_add_printf(reply, "%s", JSON);
    evhttp_send_reply(req, 200, nullptr, reply);
    evbuffer_free(reply);
}