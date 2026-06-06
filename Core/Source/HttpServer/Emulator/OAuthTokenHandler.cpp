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
#include <nlohmann/json.hpp>

using namespace NoobWarrior;

OAuthTokenHandler::OAuthTokenHandler() {}

void OAuthTokenHandler::OnRequest(evhttp_request *req, void *userdata) {
    nlohmann::json j;
    j["access_token"] = "eyJhbGciOiJFUzI1NiIsImtpZCI6Im5vb2J3YXJyaW9yIiwidHlwIjoiSldUIn0.eyJzdWIiOiI4NjEyMTg0MSIsInNjb3BlIjoib3BlbmlkIHByb2ZpbGUiLCJpc3MiOiJodHRwczovL2FwaXMucm9ibG94LmNvbS9vYXV0aC8iLCJhdWQiOiJub29id2FycmlvciJ9.";
    j["refresh_token"] = "noobwarrior_refresh";
    j["token_type"] = "Bearer";
    j["expires_in"] = 2592000;
    j["id_token"] = "eyJhbGciOiJFUzI1NiIsImtpZCI6Im5vb2J3YXJyaW9yIiwidHlwIjoiSldUIn0.eyJzdWIiOiI4NjEyMTg0MSIsIm5hbWUiOiJIYXR0b3pvIiwibmlja25hbWUiOiJIYXR0b3pvIiwicHJlZmVycmVkX3VzZXJuYW1lIjoiSGF0dG96byIsImNyZWF0ZWRfYXQiOjEsInByb2ZpbGUiOiJodHRwczovL3d3dy5yb2Jsb3guY29tL3VzZXJzLzg2MTIxODQxL3Byb2ZpbGUiLCJpc3MiOiJodHRwczovL2FwaXMucm9ibG94LmNvbS9vYXV0aC8iLCJhdWQiOiJub29id2FycmlvciJ9.";
    j["scope"] = "openid profile";

    const std::string body = j.dump();
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* buf = evbuffer_new();
    evbuffer_add_printf(buf, "%s", body.c_str());
    evhttp_send_reply(req, HTTP_OK, nullptr, buf);
    evbuffer_free(buf);
}
