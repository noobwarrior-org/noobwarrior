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

static constexpr const char* JSON = R"({"jobId":"Test","status":2,"joinScriptUrl":"http://localhost/Game/Join.ashx?placeid=1818&ip=localhost&port=53640&user=greg&id=1&membership=","authenticationUrl":"http://localhost/2021/Login/Negotiate.ashx","authenticationTicket":"1","message":null})";

using namespace NoobWarrior;

PlaceLauncherHandler::PlaceLauncherHandler() {

}

void PlaceLauncherHandler::OnRequest(evhttp_request *req, void *userdata) {
    Out("PlaceLauncherHandler", "Sending...");
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* reply = evbuffer_new();
    evbuffer_add_printf(reply, JSON);
    evhttp_send_reply(req, 200, nullptr, reply);
    evbuffer_free(reply);
}
