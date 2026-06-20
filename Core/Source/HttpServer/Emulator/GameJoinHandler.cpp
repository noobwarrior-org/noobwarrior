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
// File: GameJoinHandler.cpp
// Started by: Hattozo
// Started on: 5/26/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/GameJoinHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/Log.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Registry.h>

#include <nlohmann/json.hpp>
#if !defined(_WIN32)
#include <netinet/in.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <optional>

using namespace NoobWarrior;

GameJoinHandler::GameJoinHandler(ServerEmulator* emu) : mEmu(emu) {

}

void GameJoinHandler::OnRequest(evhttp_request *req, void *userdata) {
    // When joining a remote host, the host's emulator authors the join script (only it knows the real
    // game server address/port/place). But the joining PLAYER's identity (name, id) lives only on
    // this client, so we overlay it onto the host's join script before handing it back. The player's
    // avatar is a separate, purely-local concern (see AvatarFetchHandler).
    auto mergeIdentity = [this](std::vector<unsigned char> body) -> std::vector<unsigned char> {
        try {
            nlohmann::json j = nlohmann::json::parse(body);
            if (j.contains("joinScript") && j["joinScript"].is_object()) {
                ApplyLocalIdentity(j["joinScript"]);
                std::string merged = j.dump();
                return std::vector<unsigned char>(merged.begin(), merged.end());
            }
        } catch (const nlohmann::json::exception &e) {
            Out("GameJoinHandler", "Couldn't merge local identity into proxied join script: {}", e.what());
        }
        return body; // not JSON / no joinScript -> forward the host's response untouched
    };

    if (mEmu->TryProxyRequest(req, [this](evhttp_request *r) { HandleLocally(r); }, mergeIdentity)) {
        Out("GameJoinHandler", "Proxying join-game to joined remote emulator (with local identity)");
        return;
    }
    HandleLocally(req);
}

void GameJoinHandler::HandleLocally(evhttp_request *req) {
    const char* uri = evhttp_request_get_uri(req);
    evhttp_connection* conn = evhttp_request_get_connection(req);

    const char* peer_address = "";
    uint16_t peer_port {};
    
    if (conn != NULL)
        evhttp_connection_get_peer(conn, &peer_address, &peer_port);

    Out("GameJoinHandler", "join-game requested: {}", uri ? uri : "");

    int64_t placeId = 0;
    std::string gameJoinAttemptId = "00000000-0000-0000-0000-000000000000";
    if (evbuffer *buf = evhttp_request_get_input_buffer(req)) {
        size_t len = evbuffer_get_length(buf);
        if (len > 0) {
            std::string body(len, '\0');
            evbuffer_copyout(buf, body.data(), len);
            try {
                nlohmann::json in = nlohmann::json::parse(body);
                if (in.contains("placeId") && in["placeId"].is_number_integer())
                    placeId = in["placeId"].get<int64_t>();
                if (in.contains("gameJoinAttemptId") && in["gameJoinAttemptId"].is_string())
                    gameJoinAttemptId = in["gameJoinAttemptId"].get<std::string>();
            } catch (const nlohmann::json::exception &e) {
                Out("GameJoinHandler", "Body wasn't JSON ({}), continuing with defaults", e.what());
            }
        }
    }
    
    std::string localAddr;
    if (evhttp_connection *evcon = evhttp_request_get_connection(req)) {
        if (bufferevent *bev = evhttp_connection_get_bufferevent(evcon)) {
            evutil_socket_t fd = bufferevent_getfd(bev);
            struct sockaddr_storage addr;
            socklen_t addr_len = sizeof(addr);
            char ip_str[INET6_ADDRSTRLEN];
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
        }
    }

    std::string address = "127.0.0.1";
    uint16_t port = 53640;
    auto servers = mEmu->GetRunningGameServers();
    const RunningInstance* chosen = nullptr;
    for (const auto &s : servers) {
        if (placeId != 0 && s.PlaceId.has_value() && s.PlaceId.value() == placeId) { chosen = &s; break; }
        if (chosen == nullptr) chosen = &s;
    }
    if (chosen != nullptr) {
        if (!chosen->Ip.empty()) address = chosen->Ip;
        if (chosen->Port.has_value()) port = chosen->Port.value();
        if (placeId == 0 && chosen->PlaceId.has_value()) placeId = chosen->PlaceId.value();
    }
    if (!IsLoopbackOrEmpty(peer_address)) {
        std::string advertised = mEmu->ResolveAdvertisedAddress(localAddr);
        if (!advertised.empty()) address = advertised;
    }
    if (placeId == 0) placeId = 1818;

    Out("GameJoinHandler", "Issuing joinScript -> {}:{} placeId={}", address, port, placeId);

    nlohmann::json joinScript = {
        {"ClientPort", 0},
        {"MachineAddress", address},
        {"ServerPort", port},
        {"ServerConnections", nlohmann::json::array({ {{"Address", address}, {"Port", port}} })},
        {"DirectServerReturn", true},
        {"PepperId", 0},
        {"TokenValue", ""},
        {"PingUrl", ""},
        {"PingInterval", 120},
        {"UserName", "Player"},
        {"DisplayName", "Player"},
        {"HasVerifiedBadge", false},
        {"SeleniumTestMode", false},
        {"UserId", 1},
        {"RobloxLocale", "en_us"},
        {"GameLocale", "en_us"},
        {"SuperSafeChat", false},
        {"FlexibleChatEnabled", false},
        {"CharacterAppearance", ""},
        {"ClientTicket", "1"},
        {"GameId", gameJoinAttemptId},
        {"PlaceId", placeId},
        {"BaseUrl", "http://www.roblox.com/"},
        {"ChatStyle", "ClassicAndBubble"},
        {"CreatorId", 1},
        {"CreatorTypeEnum", "User"},
        {"MembershipType", "None"},
        {"AccountAge", 1000},
        {"CookieStoreFirstTimePlayKey", "rbx_evt_ftp"},
        {"CookieStoreFiveMinutePlayKey", "rbx_evt_fmp"},
        {"CookieStoreEnabled", false},
        {"IsUnknownOrUnder13", false},
        {"GameChatType", "AllUsers"},
        {"SessionId", gameJoinAttemptId},
        {"AnalyticsSessionId", gameJoinAttemptId},
        {"DataCenterId", 0},
        {"UniverseId", placeId},
        {"FollowUserId", 0},
        {"characterAppearanceId", 1},
        {"CountryCode", "US"},
        {"RandomSeed1", ""},
        {"ClientPublicKeyData", "Test"},
        {"PrivateServerOwnerID", 0},
        {"PrivateServerID", ""},
    };

    // Stamp this client's local player identity over the defaults above.
    ApplyLocalIdentity(joinScript);

    nlohmann::json response = {
        {"jobId", gameJoinAttemptId},
        {"status", 2}, // 2 == Done/ready to join
        {"joinScriptUrl", nullptr},
        {"authenticationUrl", "http://www.roblox.com/Login/Negotiate.ashx"},
        {"authenticationTicket", "1"},
        {"message", nullptr},
        {"joinScript", joinScript},
    };

    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* reply = evbuffer_new();
    std::string body = response.dump();
    evbuffer_add(reply, body.data(), body.size());
    evhttp_send_reply(req, 200, nullptr, reply);
    evbuffer_free(reply);
}

void GameJoinHandler::ApplyLocalIdentity(nlohmann::json &joinScript) {
    Registry* reg = mEmu->GetCore()->GetRegistry();
    if (reg == nullptr)
        return;

    int64_t userId      = reg->GetKeyValue<int64_t>("user.id").value_or(1000);
    std::string name    = reg->GetKeyValue<std::string>("user.name").value_or("Player");
    std::string display = reg->GetKeyValue<std::string>("user.display_name").value_or(name);

    joinScript["UserId"] = userId;
    joinScript["UserName"] = name;
    joinScript["DisplayName"] = display;
    // characterAppearanceId is the user id the engine fetches the avatar for; keep it consistent with
    // UserId so /v1/avatar-fetch?userId=<UserId> resolves to this same local player.
    joinScript["characterAppearanceId"] = userId;
}
