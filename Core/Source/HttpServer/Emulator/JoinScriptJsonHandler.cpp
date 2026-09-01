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
// File: JoinScriptJsonHandler.cpp
// Started by: Hattozo
// Started on: 3/22/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/JoinScriptJsonHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/HttpServer/Emulator/AuthUtil.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Registry.h>
#include <NoobWarrior/Log.h>

#include <nlohmann/json.hpp>

#include <optional>

using namespace NoobWarrior;

JoinScriptJsonHandler::JoinScriptJsonHandler(ServerEmulator* emu) : mEmu(emu) {

}

void JoinScriptJsonHandler::OnRequest(evhttp_request *req, void *userdata) {
    const char* uri = evhttp_request_get_uri(req);
    evhttp_connection* conn = evhttp_request_get_connection(req);

    const char* peer_address = "";
    uint16_t peer_port {};

    if (conn != NULL)
        evhttp_connection_get_peer(conn, &peer_address, &peer_port);
    mCore->Out("JoinScriptJsonHandler", "{}:{} requested {}", peer_address, peer_port, uri);

    evkeyvalq headers;
    if (evhttp_parse_query(uri, &headers) != 0) {
        evhttp_send_error(req, 500, "Failed to parse URL parameters");
        return;
    }
    ScopedHeaders headersGuard(&headers);

    const char* ipStr = evhttp_find_header(&headers, "ip");
    const char* portStr = evhttp_find_header(&headers, "port");
    const char* placeIdStr = evhttp_find_header(&headers, "placeId");

    std::string ipCppStr = ipStr == nullptr ? "localhost" : ipStr;
    int port = portStr == nullptr ? 53640 : strtol(portStr, nullptr, 10);
    int64_t placeId = placeIdStr == nullptr || *placeIdStr == '\0' ? 1818 : strtoll(placeIdStr, nullptr, 10);
    const int64_t universeId = mEmu->GetCore()->GetEmuDbManager()
        ->GetUniverseIdForPlace(placeId).value_or(placeId);

    // Resolve the joining player's identity and the ticket the game server will redeem. The legacy
    // captured sample values below are overwritten with these before the script is sent.
    Registry *reg = mEmu->GetCore()->GetRegistry();
    bool authEnabled = reg != nullptr && reg->GetKeyValue<bool>("emu.auth.enabled").value_or(false);

    int64_t identId = reg != nullptr ? reg->GetKeyValue<int64_t>("user.id").value_or(1000) : 1000;
    std::string identName = reg != nullptr ? reg->GetKeyValue<std::string>("user.name").value_or("Player") : "Player";
    std::string identDisplay = reg != nullptr ? reg->GetKeyValue<std::string>("user.display_name").value_or(identName) : identName;
    std::string clientTicket = "1";

    if (authEnabled) {
        EmuDb *master = mEmu->GetCore()->GetEmuDbManager()->GetMasterDatabase();

        std::optional<AuthUtil::SessionUser> user = mEmu->ResolveJoiningUser(req);
        if (!user && reg->GetKeyValue<bool>("emu.auth.allow_guests").value_or(false))
            user = AuthUtil::MakeGuestUser();

        if (!user) {
            mCore->Out("JoinScriptJsonHandler", "Refused join: authentication required and guests disabled");
            evhttp_send_error(req, HTTP_FORBIDDEN, "Authentication required to join this server");
            return;
        }

        identId = user->id;
        identName = user->name;
        identDisplay = user->displayName;
        clientTicket = user->isGuest     ? AuthUtil::EncodeGuestTicket(*user)
                     : user->isFederated ? AuthUtil::EncodeFederatedTicket(*user)
                                         : AuthUtil::MintAuthTicket(master, user->id, placeId);
        if (clientTicket.empty()) {
            evhttp_send_error(req, HTTP_INTERNAL, "Failed to mint authentication ticket");
            return;
        }
    }

    nlohmann::json joinScript = nlohmann::json::object();
    joinScript["ClientPort"] = 0;
    joinScript["MachineAddress"] = ipCppStr;
    joinScript["ServerPort"] = port;
    joinScript["ServerConnections"] = nlohmann::json::array();

    nlohmann::json serverConnection = nlohmann::json::object();
    serverConnection["Address"] = ipCppStr;
    serverConnection["Port"] = port;

    joinScript["ServerConnections"].push_back(serverConnection);
    joinScript["DirectServerReturn"] = true;
    joinScript["PingUrl"] = "https://assetgame.roblox.com/Game/ClientPresence.ashx?version=old&PlaceID=1818&GameID=29fd9df4-4c59-4d8c-8cee-8f187b09709b&UserID=7601610";
    joinScript["PingInterval"] = 120;
    joinScript["UserName"] = identName;
    joinScript["DisplayName"] = identDisplay;
    joinScript["SeleniumTestMode"] = false;
    joinScript["UserId"] = identId;
    joinScript["RobloxLocale"] = "en_us";
    joinScript["GameLocale"] = "en_us#RobloxTranslateAbTest2";
    joinScript["SuperSafeChat"] = false;
    joinScript["CharacterAppearance"] = "";
    joinScript["ClientTicket"] = clientTicket;
    joinScript["GameId"] = "29fd9df4-4c59-4d8c-8cee-8f187b09709b";
    joinScript["PlaceId"] = placeId;
    joinScript["BaseUrl"] = "http://assetgame.roblox.com/";
    joinScript["ChatStyle"] = "ClassicAndBubble";
    joinScript["CreatorId"] = 4372130;
    joinScript["CreatorTypeEnum"] = "Group";
    joinScript["MembershipType"] = "None";
    joinScript["AccountAge"] = 1859;
    joinScript["CookieStoreFirstTimePlayKey"] = "rbx_evt_ftp";
    joinScript["CookieStoreFiveMinutePlayKey"] = "rbx_evt_fmp";
    joinScript["CookieStoreEnabled"] = true;
    joinScript["IsUnknownOrUnder13"] = false;
    joinScript["GameChatType"] = "AllUsers";
    joinScript["SessionId"] = "{\"SessionId\":\"c89589f1-d1de-46e3-80e0-2703d1159409\",\"GameId\":\"29fd9df4-4c59-4d8c-8cee-8f187b09709b\",\"PlaceId\":1818,\"ClientIpAddress\":\"207.241.232.186\",\"PlatformTypeId\":5,\"SessionStarted\":\"2022-03-26T05:13:05.762819Z\",\"BrowserTrackerId\":129849985826,\"PartyId\":null,\"Age\":80.2683342765271,\"Latitude\":37.78,\"Longitude\":-122.465,\"CountryId\":1,\"PolicyCountryId\":null,\"LanguageId\":41,\"BlockedPlayerIds\":[],\"JoinType\":\"MatchMade\",\"PlaySessionFlags\":0,\"MatchmakingDecisionId\":\"a0311216-ec21-4b5d-b3c0-8538a9a4dc7d\",\"UserScoreObfuscated\":4895515560,\"UserScorePublicKey\":235,\"GameJoinMetadata\":{\"JoinSource\":0,\"RequestType\":0},\"RandomSeed2\":\"7HOfysTid4XsV/3mBPPPhKHIykE4GXSBBBzd93rplbDQ3bNSgPFcR9auB780LjNYg+4mbNQPOqTmJ2o3hUefmw==\",\"IsUserVoiceChatEnabled\":false,\"SourcePlaceId\":null}";
    joinScript["AnalyticsSessionId"] = "c89589f1-d1de-46e3-80e0-2703d1159409";
    joinScript["DataCenterId"] = 302;
    joinScript["UniverseId"] = universeId;
    joinScript["FollowUserId"] = 0;
    joinScript["characterAppearanceId"] = identId;
    joinScript["CountryCode"] = "US";
    joinScript["RandomSeed1"] = "7HOfysTid4XsV/3mBPPPhKHIykE4GXSBBBzd93rplbDQ3bNSgPFcR9auB780LjNYg+4mbNQPOqTmJ2o3hUefmw==";
    joinScript["ClientPublicKeyData"] = "{\"creationTime\":\"19:56 11/23/2021\",\"applications\":{\"RakNetEarlyPublicKey\":{\"versions\":[{\"id\":2,\"value\":\"HwatfCnkndvyKCMPSa0VAl2M2c0GQv9+0z0kENhcj2w=\",\"allowed\":true}],\"send\":2,\"revert\":2}}}";

    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "text/plain");
    evbuffer* reply = evbuffer_new();
    // evbuffer_add_printf(reply, "%s", JSON);
    std::string sig = "\r\n";
    std::string body = sig + joinScript.dump();
    evbuffer_add(reply, body.data(), body.size());
    evhttp_send_reply(req, 200, nullptr, reply);
    evbuffer_free(reply);
}
