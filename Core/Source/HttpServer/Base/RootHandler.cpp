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
// File: RootHandler.cpp
// Started by: Hattozo
// Started on: 9/13/2025
// Description:
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/HttpServer/Base/RootHandler.h>
#include <NoobWarrior/HttpServer/Base/HttpServer.h>

using namespace NoobWarrior;

RootHandler::RootHandler(HttpServer *server) : mServer(server) {
    
}

void RootHandler::OnRequest(evhttp_request* req, void *userdata) {
    bool sentReply = false;
    const char* uri = evhttp_request_get_uri(req);
    evhttp_connection* conn = evhttp_request_get_connection(req);

    const char* peer_address = "";
    uint16_t peer_port {};
    
    if (conn != NULL)
        evhttp_connection_get_peer(conn, &peer_address, &peer_port);

    sol::table reqTbl = mServer->GetCore()->GetLuaState()->create_table();
    reqTbl["Uri"] = uri;
    reqTbl["PeerIp"] = peer_address;
    reqTbl["PeerPort"] = peer_port;
    reqTbl["AddHeader"] = [req](sol::table self, std::string key, std::string val) {
        evhttp_add_header(evhttp_request_get_output_headers(req), key.c_str(), val.c_str());
    };
    reqTbl["SendReply"] = [req, &sentReply](sol::table self, int code, std::string reason, std::string data) {
        if (sentReply)
            return;
        evbuffer *reply = evbuffer_new();
        evbuffer_add_printf(reply, "%s", data.c_str());
        evhttp_send_reply(req, code, reason.c_str(), reply);
        evbuffer_free(reply);
        sentReply = true;
    };
    reqTbl["SendError"] = [req, &sentReply](sol::table self, int error, std::string reason) {
        if (sentReply)
            return;
        evhttp_send_error(req, error, reason.c_str());
        sentReply = true;
    };
    mServer->GetOnRequestSignal()->Fire(reqTbl);
    if (!sentReply)
        evhttp_send_reply(req, HTTP_OK, nullptr, nullptr);
}