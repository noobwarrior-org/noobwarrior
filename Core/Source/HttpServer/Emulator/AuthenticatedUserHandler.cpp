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
// File: AuthenticatedUserHandler.cpp
// Started by: Hattozo
// Started on: 4/21/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/AuthenticatedUserHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/HttpServer/Emulator/AuthUtil.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Registry.h>
#include <NoobWarrior/Log.h>
#include <nlohmann/json.hpp>

using namespace NoobWarrior;

AuthenticatedUserHandler::AuthenticatedUserHandler(ServerEmulator* emu) : mEmu(emu) {

}

void AuthenticatedUserHandler::OnRequest(evhttp_request *req, void *userdata) {
    Out("AuthenticatedUserHandler", "Sent!");

    auto* registry = mEmu->GetCore()->GetRegistry();
    auto id = registry->GetKeyValue<int64_t>("user.id").value_or(1);
    auto name = registry->GetKeyValue<std::string>("user.name").value_or("Player");
    auto displayName = registry->GetKeyValue<std::string>("user.display_name").value_or("Player");

    // Under auth, report the authenticated joining player instead of the local registry identity.
    if (registry->GetKeyValue<bool>("emu.auth.enabled").value_or(false)) {
        if (auto user = mEmu->ResolveJoiningUser(req)) {
            id = user->id;
            name = user->name;
            displayName = user->displayName;
        }
    }

    nlohmann::json j;
    j["id"] = id;
    j["name"] = name;
    j["displayName"] = displayName;

    const std::string body = j.dump();
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* reply = evbuffer_new();
    evbuffer_add_printf(reply, "%s", body.c_str());
    evhttp_send_reply(req, 200, nullptr, reply);
    evbuffer_free(reply);
}
