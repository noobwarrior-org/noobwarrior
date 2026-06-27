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
// File: OAuthUserInfoHandler.cpp
// Started by: Hattozo
// Started on: 6/6/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/OAuthUserInfoHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/NoobWarrior.h>
#include <nlohmann/json.hpp>

using namespace NoobWarrior;

OAuthUserInfoHandler::OAuthUserInfoHandler(ServerEmulator* emu) : mEmu(emu) {}

void OAuthUserInfoHandler::OnRequest(evhttp_request *req, void *userdata) {
    auto name = mEmu->GetCore()->GetRegistry()->GetKeyValue<std::string>("user.name").value_or("Player");
    auto displayname = mEmu->GetCore()->GetRegistry()->GetKeyValue<std::string>("user.display_name").value_or("Player");
    auto id = mEmu->GetCore()->GetRegistry()->GetKeyValue<int64_t>("user.id").value_or(1);
    
    nlohmann::json j;
    j["sub"] = std::to_string(id);
    j["name"] = name;
    j["nickname"] = displayname;
    j["preferred_username"] = displayname;
    j["created_at"] = 1;
    j["profile"] = "https://www.roblox.com/users/" + std::to_string(id) + "/profile";
    j["picture"] = "https://www.roblox.com/headshots/default.png";
    j["age_bracket"] = "Age13OrOver";
    j["premium"] = false;
    j["roles"] = nlohmann::json::array();
    j["internal_user"] = false;

    const std::string body = j.dump();
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* buf = evbuffer_new();
    evbuffer_add_printf(buf, "%s", body.c_str());
    evhttp_send_reply(req, HTTP_OK, nullptr, buf);
    evbuffer_free(buf);
}
