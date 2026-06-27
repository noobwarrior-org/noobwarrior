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
// File: OAuthTokenHandler.cpp
// Started by: Hattozo
// Started on: 6/6/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/OAuthTokenHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/NoobWarrior.h>
#include <nlohmann/json.hpp>

#include "../../algorithm/base64.h"

using namespace NoobWarrior;

namespace {
std::string Base64Url(const std::string &in) {
    std::string s = base64_encode(reinterpret_cast<const BYTE*>(in.data()),
                                  static_cast<unsigned int>(in.size()));
    for (char &c : s) {
        if (c == '+') c = '-';
        else if (c == '/') c = '_';
    }
    s.erase(s.find_last_not_of('=') + 1);
    return s;
}

std::string MakeJwt(const nlohmann::json &payload) {
    static const std::string header =
        Base64Url(R"({"alg":"ES256","kid":"noobwarrior","typ":"JWT"})");
    return header + "." + Base64Url(payload.dump()) + ".";
}
}

OAuthTokenHandler::OAuthTokenHandler(ServerEmulator* emu) : mEmu(emu) {}

void OAuthTokenHandler::OnRequest(evhttp_request *req, void *userdata) {
    auto* registry = mEmu->GetCore()->GetRegistry();
    auto id = registry->GetKeyValue<int64_t>("user.id").value_or(1);
    auto name = registry->GetKeyValue<std::string>("user.name").value_or("Player");
    auto displayName = registry->GetKeyValue<std::string>("user.display_name").value_or("Player");
    const std::string sub = std::to_string(id);

    nlohmann::json accessPayload = {
        {"sub", sub},
        {"scope", "openid profile"},
        {"iss", "https://apis.roblox.com/oauth/"},
        {"aud", "noobwarrior"},
    };

    nlohmann::json idPayload = {
        {"sub", sub},
        {"name", name},
        {"nickname", displayName},
        {"preferred_username", displayName},
        {"created_at", 1},
        {"profile", "https://www.roblox.com/users/" + sub + "/profile"},
        {"iss", "https://apis.roblox.com/oauth/"},
        {"aud", "noobwarrior"},
    };

    nlohmann::json j;
    j["access_token"] = MakeJwt(accessPayload);
    j["refresh_token"] = "noobwarrior_refresh";
    j["token_type"] = "Bearer";
    j["expires_in"] = 2592000;
    j["id_token"] = MakeJwt(idPayload);
    j["scope"] = "openid profile";

    const std::string body = j.dump();
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* buf = evbuffer_new();
    evbuffer_add_printf(buf, "%s", body.c_str());
    evhttp_send_reply(req, HTTP_OK, nullptr, buf);
    evbuffer_free(buf);
}
