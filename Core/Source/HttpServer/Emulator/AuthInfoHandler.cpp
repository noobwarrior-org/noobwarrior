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
// File: AuthInfoHandler.cpp
// Started by: Hattozo
// Started on: 7/2/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/AuthInfoHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Registry.h>

#include <nlohmann/json.hpp>

using namespace NoobWarrior;

AuthInfoHandler::AuthInfoHandler(ServerEmulator* emu) : mEmu(emu) {

}

void AuthInfoHandler::OnRequest(evhttp_request *req, void *userdata) {
    Registry* reg = mEmu->GetCore()->GetRegistry();

    nlohmann::json j;
    j["authEnabled"]       = reg->GetKeyValue<bool>("emu.auth.enabled").value_or(false);
    j["passwordBased"]     = reg->GetKeyValue<bool>("emu.auth.password_based").value_or(true);
    j["allowGuests"]       = reg->GetKeyValue<bool>("emu.auth.allow_guests").value_or(false);
    j["allowRegistration"] = reg->GetKeyValue<bool>("emu.auth.allow_registration").value_or(false);
    j["ticketTtl"]         = reg->GetKeyValue<int64_t>("emu.auth.ticket_ttl").value_or(120);
    j["branding"]["title"]   = reg->GetKeyValue<std::string>("emu.branding.title").value_or("noobWarrior Server");
    j["branding"]["tagline"] = reg->GetKeyValue<std::string>("emu.branding.tagline").value_or("");
    j["branding"]["icon"]    = reg->GetKeyValue<std::string>("emu.branding.icon").value_or("");

    std::string body = j.dump();
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* reply = evbuffer_new();
    evbuffer_add(reply, body.data(), body.size());
    evhttp_send_reply(req, HTTP_OK, nullptr, reply);
    evbuffer_free(reply);
}
