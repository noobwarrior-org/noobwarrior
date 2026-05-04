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

#include <sstream>

using namespace NoobWarrior;

CreateAccountHandler::CreateAccountHandler(ServerEmulator* emu) : mEmu(emu) {

}

void CreateAccountHandler::OnRequest(evhttp_request *req, void *userdata) {
    std::map<std::string, std::string> params = GetPostFormParameters(req);
    for (auto& [k, v] : params) {
        Out("CreateAccountHandler", "{} {}", k, v);
    }
    evhttp_send_error(req, 500, "oops");
}
