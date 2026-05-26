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

    evkeyvalq* headers = evhttp_request_get_input_headers(req);
    

    Out("RootHandler", "{}:{} requested URI {}", peer_address, peer_port, uri);

    sol::table reqTbl = mServer->GetCore()->GetLuaState()->create_table();
    reqTbl["Uri"] = uri;
    reqTbl["PeerIp"] = peer_address;
    reqTbl["PeerPort"] = peer_port;
    
    sol::table headersTbl = mServer->GetCore()->GetLuaState()->create_table();
    headersTbl["Cookie"]     = evhttp_find_header(headers, "Cookie");
    headersTbl["User-Agent"] = evhttp_find_header(headers, "User-Agent");
    headersTbl["Host"]       = evhttp_find_header(headers, "Host");
    reqTbl["Headers"] = headersTbl;

    evhttp_cmd_type method = evhttp_request_get_command(req);
    const char* method_str = "GET";
    switch (method) {
    case EVHTTP_REQ_POST:    method_str = "POST";    break;
    case EVHTTP_REQ_HEAD:    method_str = "HEAD";    break;
    case EVHTTP_REQ_PUT:     method_str = "PUT";     break;
    case EVHTTP_REQ_DELETE:  method_str = "DELETE";  break;
    case EVHTTP_REQ_OPTIONS: method_str = "OPTIONS"; break;
    case EVHTTP_REQ_PATCH:   method_str = "PATCH";   break;
    default: break;
    }
    reqTbl["Method"] = method_str;

    reqTbl["AddHeader"] = [req](sol::table self, std::string key, std::string val) {
        evhttp_add_header(evhttp_request_get_output_headers(req), key.c_str(), val.c_str());
    };
    reqTbl["SendReply"] = [req, &sentReply](sol::table self, int code, std::string reason, std::string data) {
        if (sentReply)
            return;
        evbuffer *reply = evbuffer_new();
        evbuffer_add(reply, data.data(), data.size());
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
    if (method == EVHTTP_REQ_POST) {
        struct evbuffer *buf = evhttp_request_get_input_buffer(req);
        size_t len = evbuffer_get_length(buf);

        char *data = new char[len + 1];
        evbuffer_copyout(buf, data, len);
        data[len] = '\0';

        reqTbl["PostBody"] = std::string(data);

        delete[] data;
    }
    mServer->GetOnRequestSignal()->Fire(reqTbl);
    if (!sentReply)
        evhttp_send_reply(req, HTTP_OK, nullptr, nullptr);
}