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
// File: SessionCheckHandler.cpp
// Started by: Hattozo
// Started on: 7/3/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/SessionCheckHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/HttpServer/Emulator/AuthUtil.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Registry.h>
#include <nlohmann/json.hpp>

using namespace NoobWarrior;

SessionCheckHandler::SessionCheckHandler(ServerEmulator* emu) : mEmu(emu) {}

void SessionCheckHandler::OnRequest(evhttp_request *req, void *userdata) {
    const char *cookie = evhttp_find_header(evhttp_request_get_input_headers(req), "Cookie");
    std::string token = AuthUtil::ExtractCookieValue(cookie, ".LOGINSESSION");

    Registry *reg = mEmu->GetCore()->GetRegistry();
    int64_t ttlSeconds = reg != nullptr
        ? reg->GetKeyValue<int64_t>("emu.auth.session_ttl_days").value_or(30) * 86400
        : 0;

    EmuDb *master = mEmu->GetCore()->GetEmuDbManager()->GetMasterDatabase();
    std::optional<AuthUtil::SessionUser> user = AuthUtil::ResolveSessionUser(master, token, ttlSeconds);
    if (!user) {
        evhttp_send_error(req, 401, "Session is not valid");
        return;
    }

    nlohmann::json j;
    j["id"] = user->id;
    j["name"] = user->name;
    j["displayName"] = user->displayName;
    std::string body = j.dump();

    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer *reply = evbuffer_new();
    evbuffer_add(reply, body.data(), body.size());
    evhttp_send_reply(req, 200, nullptr, reply);
    evbuffer_free(reply);
}
