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
// File: OAuthAuthorizeHandler.cpp
// Started by: Hattozo
// Started on: 5/25/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/OAuthAuthorizeHandler.h>
#include <NoobWarrior/Log.h>

#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/keyvalq_struct.h>

using namespace NoobWarrior;

OAuthAuthorizeHandler::OAuthAuthorizeHandler() {}

void OAuthAuthorizeHandler::OnRequest(evhttp_request *req, void *userdata) {
    const char *uri = evhttp_request_get_uri(req);
    Out("OAuthAuthorizeHandler", "Authorize request: {}", uri ? uri : "");

    std::string state;
    evhttp_uri *parsed = uri ? evhttp_uri_parse(uri) : nullptr;
    if (parsed != nullptr) {
        const char *query = evhttp_uri_get_query(parsed);
        if (query != nullptr) {
            evkeyvalq params {};
            if (evhttp_parse_query_str(query, &params) == 0) {
                const char *s = evhttp_find_header(&params, "state");
                if (s != nullptr) state = s;
                evhttp_clear_headers(&params);
            }
        }
        evhttp_uri_free(parsed);
    }

    std::string location = "roblox-studio-auth:/?code=a";
    if (!state.empty()) {
        location += "&state=";
        location += state;
    }

    evhttp_add_header(evhttp_request_get_output_headers(req), "Location", location.c_str());
    evbuffer *reply = evbuffer_new();
    evhttp_send_reply(req, 302, "Found", reply);
    evbuffer_free(reply);
}
