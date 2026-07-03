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
// File: PlaceLauncherHandler.cpp
// Started by: Hattozo
// Started on: 3/22/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/PlaceLauncherHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/HttpServer/Emulator/AuthUtil.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Registry.h>
#include <NoobWarrior/Log.h>

#include <nlohmann/json.hpp>

static constexpr const char* JSON = R"({"jobId":"Test","status":2,"joinScriptUrl":"http://localhost/Game/Join.ashx?placeid=1818&ip=localhost&port=53640&user=greg&id=1&membership=","authenticationUrl":"http://localhost/2021/Login/Negotiate.ashx","authenticationTicket":"1","message":null})";

using namespace NoobWarrior;

PlaceLauncherHandler::PlaceLauncherHandler(ServerEmulator* emu) : mEmu(emu) {

}

void PlaceLauncherHandler::OnRequest(evhttp_request *req, void *userdata) {
    const char* uri = evhttp_request_get_uri(req);
    evhttp_connection* conn = evhttp_request_get_connection(req);

    const char* peer_address = "";
    uint16_t peer_port {};

    if (conn != NULL)
        evhttp_connection_get_peer(conn, &peer_address, &peer_port);
    Out("PlaceLauncherHandler", "{}:{} requested {}", peer_address, peer_port, uri);

    evkeyvalq headers;
    if (evhttp_parse_query(uri, &headers) != 0) {
        evhttp_send_error(req, 500, "Failed to parse URL parameters");
        return;
    }
    ScopedHeaders headersGuard(&headers);

    const char* ipStr = evhttp_find_header(&headers, "ip");
    const char* portStr = evhttp_find_header(&headers, "port");
    const char* placeIdStr = evhttp_find_header(&headers, "placeId");

    // Auth gate: refuse to hand out a join script when auth is required and there's no joining
    // identity and no guest access. The identity itself is resolved (and the ticket minted) by the
    // join-script handler; the client authenticates separately via /v1/authentication-ticket/redeem.
    if (Registry *reg = mEmu->GetCore()->GetRegistry();
        reg != nullptr && reg->GetKeyValue<bool>("emu.auth.enabled").value_or(false)) {
        bool allowGuests = reg->GetKeyValue<bool>("emu.auth.allow_guests").value_or(false);
        if (!mEmu->ResolveJoiningUser(req) && !allowGuests) {
            nlohmann::json denied = nlohmann::json::object();
            denied["jobId"] = "Test";
            denied["status"] = 22; // non-2: the client treats this as a failed launch
            denied["joinScriptUrl"] = nullptr;
            denied["authenticationUrl"] = "http://www.roblox.com/Login/Negotiate.ashx";
            denied["authenticationTicket"] = nullptr;
            denied["message"] = "Authentication required to join this server";
            evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
            evbuffer *deny = evbuffer_new();
            evbuffer_add_printf(deny, "%s", denied.dump().c_str());
            evhttp_send_reply(req, 200, nullptr, deny);
            evbuffer_free(deny);
            return;
        }
    }

    nlohmann::json json = nlohmann::json::object();
    json["jobId"] = "Test";
    json["status"] = 2;
    json["joinScriptUrl"] = std::format("http://www.roblox.com/Game/Join.ashx?ip={}&port={}&placeId={}",
        ipStr == nullptr ? "" : ipStr,
        portStr == nullptr ? "" : portStr,
        placeIdStr == nullptr ? "" : placeIdStr);
    json["authenticationUrl"] = "http://www.roblox.com/Login/Negotiate.ashx";
    // json["joinScriptUrl"] = "http://localhost/2021/game/join.ashx?placeid=1818&ip=localhost&port=53640&user=greg&id=7601610&membership=&app=http://localhost/charscript/Custom.php?hat=0;password=7601610|Pastel brown;Cyan;Pastel brown;Pastel brown;Cyan;Cyan";
    // json["authenticationUrl"] = "http://localhost/2021/Login/Negotiate.ashx";
    json["authenticationTicket"] = "1";
    json["message"] = nullptr;

    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* reply = evbuffer_new();
    evbuffer_add_printf(reply, "%s", json.dump().c_str());
    // evbuffer_add_printf(reply, "%s", JSON);
    evhttp_send_reply(req, 200, nullptr, reply);
    evbuffer_free(reply);
}
