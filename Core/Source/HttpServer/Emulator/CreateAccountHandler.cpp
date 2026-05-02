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
// File: CreateAccountHandler.h
// Started by: Hattozo
// Started on: 5/1/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/CreateAccountHandler.h>
#include <NoobWarrior/Log.h>

using namespace NoobWarrior;

CreateAccountHandler::CreateAccountHandler(ServerEmulator* emu) : mEmu(emu) {

}

void CreateAccountHandler::OnRequest(evhttp_request *req, void *userdata) {
    evhttp_cmd_type method = evhttp_request_get_command(req);
    if (method == EVHTTP_REQ_POST) {
        struct evbuffer *buf = evhttp_request_get_input_buffer(req);
        size_t len = evbuffer_get_length(buf);

        char *data = new char[len + 1];
        evbuffer_copyout(buf, data, len);
        data[len] = '\0';
        Out("CreateAccountHandler", data);
        delete[] data;
    }
}
