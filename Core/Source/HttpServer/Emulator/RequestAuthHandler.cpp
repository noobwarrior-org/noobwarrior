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
// File: RequestAuthHandler.cpp
// Started by: Hattozo
// Started on: 4/21/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/RequestAuthHandler.h>
#include <NoobWarrior/NoobWarrior.h>

using namespace NoobWarrior;

RequestAuthHandler::RequestAuthHandler() {

}

void RequestAuthHandler::OnRequest(evhttp_request *req, void *userdata) {
    mCore->Out("RequestAuthHandler", "Authenticating...");
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "text/plain");
    evbuffer* reply = evbuffer_new();
    evbuffer_add_printf(reply, "true");
    evhttp_send_reply(req, 200, nullptr, reply);
    evbuffer_free(reply);
}
