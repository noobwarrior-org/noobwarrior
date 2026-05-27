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

#include <nlohmann/json.hpp>

#include <optional>

using namespace NoobWarrior;

GameJoinHandler::GameJoinHandler(ServerEmulator* emu) : mEmu(emu) {

}

void GameJoinHandler::OnRequest(evhttp_request *req, void *userdata) {
    const char* uri = evhttp_request_get_uri(req);
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
