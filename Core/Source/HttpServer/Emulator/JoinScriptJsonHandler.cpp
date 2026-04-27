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
#include <NoobWarrior/Log.h>

#include <nlohmann/json.hpp>

using namespace NoobWarrior;

JoinScriptJsonHandler::JoinScriptJsonHandler() {

}

void JoinScriptJsonHandler::OnRequest(evhttp_request *req, void *userdata) {
    const char* uri = evhttp_request_get_uri(req);
    evhttp_connection* conn = evhttp_request_get_connection(req);

    const char* peer_address = "";
    uint16_t peer_port {};

    if (conn != NULL)
        evhttp_connection_get_peer(conn, &peer_address, &peer_port);
    Out("JoinScriptJsonHandler", "{}:{} requested {}", peer_address, peer_port, uri);

    evkeyvalq headers;
    if (evhttp_parse_query(uri, &headers) != 0) {
        evhttp_send_error(req, 500, "Failed to parse URL parameters");
        return;
    }

    const char* ipStr = evhttp_find_header(&headers, "ip");
    const char* portStr = evhttp_find_header(&headers, "port");
    const char* localStr = evhttp_find_header(&headers, "local");

    std::string ipCppStr = ipStr == nullptr ? "localhost" : ipStr;
    int port = portStr == nullptr ? 53640 : strtol(portStr, nullptr, 10);
    std::string localCppStr = localStr == nullptr ? "{}" : localStr;

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
    joinScript["UserName"] = "heat tozo";
    joinScript["DisplayName"] = "heat tozo display name";
    joinScript["SeleniumTestMode"] = false;
    joinScript["UserId"] = 7601610;
    joinScript["RobloxLocale"] = "en_us";
    joinScript["GameLocale"] = "en_us#RobloxTranslateAbTest2";
    joinScript["SuperSafeChat"] = false;
    joinScript["CharacterAppearance"] = "";
    joinScript["ClientTicket"] = "2022-03-26T05:13:05.7649319Z;dj09X5iTmYtOPwh0hbEC8yvSO1t99oB3Yh5qD/sinDFszq3hPPaL6hH16TvtCen6cABIycyDv3tghW7k8W+xuqW0/xWvs0XJeiIWstmChYnORzM1yCAVnAh3puyxgaiIbg41WJSMALRSh1hoRiVFOXw4BKjSKk7DrTTcL9nOG1V5YwVnmAJKY7/m0yZ81xE99QL8UVdKz2ycK8l8JFvfkMvgpqLNBv0APRNykGDauEhAx283vARJFF0D9UuSV69q6htLJ1CN2kXL0Saxtt/kRdoP3p3Nhj2VgycZnGEo2NaG25vwc/KzOYEFUV0QdQPC8Vs2iFuq8oK+fXRc3v6dnQ==;BO8oP7rzmnIky5ethym6yRECd6H14ojfHP3nHxSzfTs=;XsuKZL4TBjh8STukr1AgkmDSo5LGgQKQbvymZYi/80TYPM5/MXNr5HKoF3MOT3Nfm0MrubracyAtg5O3slIKBg==;6";
    joinScript["GameId"] = "29fd9df4-4c59-4d8c-8cee-8f187b09709b";
    joinScript["PlaceId"] = 1818;
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
    joinScript["UniverseId"] = 994732206;
    joinScript["FollowUserId"] = 0;
    joinScript["characterAppearanceId"] = 244775698;
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
