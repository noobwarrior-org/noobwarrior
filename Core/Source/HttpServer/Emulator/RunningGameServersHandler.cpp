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
// File: RunningGameServersHandler.cpp
// Started by: Hattozo
// Started on: 4/24/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/RunningGameServersHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/Log.h>
#include <NoobWarrior/NoobWarrior.h>

#include <nlohmann/json.hpp>
#if !defined(_WIN32)
#include <netinet/in.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

using namespace NoobWarrior;

RunningGameServersHandler::RunningGameServersHandler(ServerEmulator* emu) : mEmu(emu) {

}

void RunningGameServersHandler::OnRequest(evhttp_request *req, void *userdata) {
    mCore->Out("RunningGameServersHandler", "Yes.");

    struct evhttp_connection *evcon = evhttp_request_get_connection(req);
    struct bufferevent *bev = evhttp_connection_get_bufferevent(evcon);
    evutil_socket_t fd = bufferevent_getfd(bev);

    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);

    char ip_str[INET6_ADDRSTRLEN];
    std::string localAddr;

    if (getsockname(fd, (struct sockaddr*)&addr, &addr_len) == 0) {
        if (addr.ss_family == AF_INET) {
            struct sockaddr_in *sin = (struct sockaddr_in*)&addr;
            if (evutil_inet_ntop(AF_INET, &sin->sin_addr, ip_str, INET6_ADDRSTRLEN))
                localAddr = ip_str;
        } else if (addr.ss_family == AF_INET6) {
            struct sockaddr_in6 *sin6 = (struct sockaddr_in6*)&addr;
            if (evutil_inet_ntop(AF_INET6, &sin6->sin6_addr, ip_str, INET6_ADDRSTRLEN))
                localAddr = ip_str;
        }
    }

    std::string advertised = mEmu->ResolveAdvertisedAddress(localAddr);

    nlohmann::json json = nlohmann::json::array();
    for (const auto &server : mEmu->GetRunningGameServers()) {
        nlohmann::json obj = nlohmann::json::object();
        obj["Pid"] = server.Pid;
        obj["Ip"] = (IsLoopbackOrEmpty(server.Ip) && !advertised.empty()) ? advertised : server.Ip;
        if (server.Port.has_value()) obj["Port"] = server.Port.value();
        if (server.PlaceId.has_value()) obj["PlaceId"] = server.PlaceId.value();
        obj["EngineType"] = EngineTypeAsString(EngineType::Roblox);
        obj["EngineVersion"] = server.Version;
        obj["FirstSeen"] = server.FirstSeen;
        obj["LastSeen"] = server.LastSeen;
        json.push_back(obj);
    }

    evkeyvalq *headers = evhttp_request_get_output_headers(req);
    evhttp_add_header(headers, "Content-Type", "application/json");
    evhttp_add_header(headers, ServerEmulator::kIdentityHeader,
                      mEmu->GetInstanceId().c_str());
    evbuffer* reply = evbuffer_new();
    evbuffer_add_printf(reply, "%s", json.dump().c_str());
    evhttp_send_reply(req, 200, nullptr, reply);
    evbuffer_free(reply);
}
