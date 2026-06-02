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
// File: AuthTicketRedeemHandler.cpp
// Started by: Hattozo
// Started on: 6/1/2026
// Description: Currently a stub
#include <NoobWarrior/HttpServer/Emulator/AuthTicketRedeemHandler.h>

using namespace NoobWarrior;

AuthTicketRedeemHandler::AuthTicketRedeemHandler(ServerEmulator* emu) : mEmu(emu) {

}

void AuthTicketRedeemHandler::OnRequest(evhttp_request *req, void *userdata) {
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* stub = evbuffer_new();
    evbuffer_add(stub, "{}", 2);
    evhttp_send_reply(req, HTTP_OK, nullptr, stub);
    evbuffer_free(stub);
}
