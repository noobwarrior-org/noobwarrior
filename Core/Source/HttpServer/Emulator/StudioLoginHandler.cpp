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
// File: StudioLoginHandler.cpp
// Started by: Hattozo
// Started on: 6/6/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/StudioLoginHandler.h>
#include <nlohmann/json.hpp>

using namespace NoobWarrior;

StudioLoginHandler::StudioLoginHandler() {}

void StudioLoginHandler::OnRequest(evhttp_request *req, void *userdata) {
    nlohmann::json j;
    j["success"] = true;
    j["user"]["UserId"] = 86121841;
    j["user"]["Username"] = "Hattozo";
    j["user"]["DisplayName"] = "Hattozo";
    j["user"]["AgeBracket"] = 0;
    j["user"]["Roles"] = nlohmann::json::array();
    j["user"]["Email"]["value"] = "hattozo@noobwarrior.local";
    j["user"]["Email"]["isVerified"] = true;
    j["user"]["IsBanned"] = false;
    j["user"]["isInternal"] = false;

    const std::string body = j.dump();
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* buf = evbuffer_new();
    evbuffer_add_printf(buf, "%s", body.c_str());
    evhttp_send_reply(req, HTTP_OK, nullptr, buf);
    evbuffer_free(buf);
}
