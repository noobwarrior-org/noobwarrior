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

#include <cstring>
#include <map>
#include <string>
#include <vector>

using namespace NoobWarrior;

static std::string CollapseSlashes(const std::string &s) {
    std::string out;
    out.reserve(s.size());
    bool prevSlash = false;
    for (char c : s) {
        if (c == '/') {
            if (prevSlash) continue;
            prevSlash = true;
        } else {
            prevSlash = false;
        }
        out += c;
    }
    return out;
}

static std::vector<std::string> SplitPath(const std::string &s) {
    std::vector<std::string> segs;
    size_t start = 0;
    for (size_t i = 0; i <= s.size(); i++) {
        if (i == s.size() || s[i] == '/') {
            segs.push_back(s.substr(start, i - start));
            start = i + 1;
        }
    }
    return segs;
}

static bool MatchRoute(const std::string &pattern, const std::string &path,
                       std::map<std::string, std::string> &params) {
    if (pattern == path) return true;
    if (pattern.find(':') == std::string::npos) return false;

    std::vector<std::string> pp = SplitPath(pattern);
    std::vector<std::string> tp = SplitPath(path);
    if (pp.size() != tp.size()) return false;

    std::map<std::string, std::string> captured;
    for (size_t i = 0; i < pp.size(); i++) {
        if (!pp[i].empty() && pp[i][0] == ':') {
            if (tp[i].empty()) return false; // a :param must capture a non-empty segment
            captured[pp[i].substr(1)] = tp[i];
        } else if (pp[i] != tp[i]) {
            return false;
        }
    }
    params = std::move(captured);
    return true;
}

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

    auto logVal = mServer->GetCore()->GetRegistry()->GetKeyValue<bool>("debug.log_http_server_requests").value_or(false);
    if (logVal)
        Out(mServer->mLogName, "{}:{} requested URI {}", peer_address, peer_port, uri);

    std::string path = uri ? uri : "";
    if (auto q = path.find('?'); q != std::string::npos)
        path.resize(q);
    path = CollapseSlashes(path);

    for (const auto &entry : mServer->mStoredHandlers) {
        if (!entry.uri) continue;
        Handler *handler = std::get<0>(*entry.raw);
        if (handler == this) continue;
        std::map<std::string, std::string> params;
        if (MatchRoute(*entry.uri, path, params)) {
            mServer->mRouteParams = std::move(params);
            handler->OnRequest(req, std::get<1>(*entry.raw));
            return;
        }
    }
    mServer->mRouteParams.clear();

    sol::table reqTbl = mServer->GetCore()->GetLuaState()->create_table();
    reqTbl["Uri"] = uri;
    reqTbl["PeerIp"] = peer_address;
    reqTbl["PeerPort"] = peer_port;
    
    sol::table headersTbl = mServer->GetCore()->GetLuaState()->create_table();
    headersTbl["Cookie"]       = evhttp_find_header(headers, "Cookie");
    headersTbl["User-Agent"]   = evhttp_find_header(headers, "User-Agent");
    headersTbl["Host"]         = evhttp_find_header(headers, "Host");
    headersTbl["Content-Type"] = evhttp_find_header(headers, "Content-Type");
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
    reqTbl["RemoveHeader"] = [req](sol::table self, std::string key) {
        // evhttp keeps duplicates, so remove every existing entry for this key.
        evkeyvalq* outHeaders = evhttp_request_get_output_headers(req);
        while (evhttp_remove_header(outHeaders, key.c_str()) == 0) {}
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
        
        reqTbl["PostBody"] = std::string(data, len);

        delete[] data;
    }
    mServer->GetOnRequestSignal()->Fire(reqTbl);
    if (!sentReply)
        evhttp_send_reply(req, HTTP_OK, nullptr, nullptr);
}