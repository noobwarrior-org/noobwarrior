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
// File: AuthTicketRedeemHandler.cpp
// Started by: Hattozo
// Started on: 6/1/2026
// Description: The game server redeems a join ticket here to confirm identity
// Doesn't do much when emu.auth.enabled or whatever its called is off
#include <NoobWarrior/HttpServer/Emulator/AuthTicketRedeemHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/HttpServer/Emulator/AuthUtil.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Registry.h>
#include <NoobWarrior/Log.h>

#include <nlohmann/json.hpp>

#include <optional>
#include <string>

using namespace NoobWarrior;

AuthTicketRedeemHandler::AuthTicketRedeemHandler(ServerEmulator* emu) : mEmu(emu) {

}

// Pulls the ticket out of wherever the caller put it: the rbx-authentication-ticket header, a JSON
// body field (authenticationTicket / ticket), or the same names as a form/query field.
static std::string ExtractTicket(evhttp_request *req) {
    if (const char *hdr = evhttp_find_header(evhttp_request_get_input_headers(req), "rbx-authentication-ticket");
        hdr != nullptr && *hdr != '\0')
        return hdr;

    if (evbuffer *buf = evhttp_request_get_input_buffer(req)) {
        size_t len = evbuffer_get_length(buf);
        if (len > 0) {
            std::string body(len, '\0');
            evbuffer_copyout(buf, body.data(), len);
            try {
                nlohmann::json j = nlohmann::json::parse(body);
                for (const char *key : {"authenticationTicket", "ticket"}) {
                    if (j.contains(key) && j[key].is_string())
                        return j[key].get<std::string>();
                }
            } catch (const nlohmann::json::exception &) {
                // not JSON; fall through to form parsing
            }
        }
    }

    for (auto &kv : Handler::GetPostFormParameters(req)) {
        if (kv.first == "authenticationTicket" || kv.first == "ticket")
            return kv.second;
    }
    return "";
}

void AuthTicketRedeemHandler::OnRequest(evhttp_request *req, void *userdata) {
    auto sendJson = [req](int code, const std::string &body) {
        evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
        evbuffer *reply = evbuffer_new();
        evbuffer_add(reply, body.data(), body.size());
        evhttp_send_reply(req, code, nullptr, reply);
        evbuffer_free(reply);
    };

    Registry *reg = mEmu->GetCore()->GetRegistry();
    bool authEnabled = reg != nullptr && reg->GetKeyValue<bool>("emu.auth.enabled").value_or(false);

    // Auth off: keep the historical no-op so local, non-authenticated flows are unaffected.
    if (!authEnabled) {
        sendJson(HTTP_OK, "{}");
        return;
    }

    bool allowGuests = reg != nullptr && reg->GetKeyValue<bool>("emu.auth.allow_guests").value_or(false);
    int64_t ttl = reg != nullptr ? reg->GetKeyValue<int64_t>("emu.auth.ticket_ttl").value_or(120) : 120;

    EmuDb *master = mEmu->GetCore()->GetEmuDbManager()->GetMasterDatabase();
    std::string ticket = ExtractTicket(req);

    std::optional<AuthUtil::SessionUser> resolved;

    // Guests and federated users carry their identity inside the ticket itself (no DB row exists for
    // them); a local account gets a real single-use ticket redeemed against the master DB.
    if (auto guest = AuthUtil::DecodeGuestTicket(ticket)) {
        resolved = guest;
    } else if (auto federated = AuthUtil::DecodeFederatedTicket(ticket)) {
        resolved = federated;
    } else if (master) {
        resolved = AuthUtil::RedeemAuthTicket(master, ticket, ttl);
    }

    // A missing/expired/used ticket is treated as a guest when guests are allowed, otherwise refused.
    if (!resolved) {
        if (allowGuests) {
            resolved = AuthUtil::MakeGuestUser();
        } else {
            Out("AuthTicketRedeemHandler", "Refused redeem of ticket (invalid/expired and guests disabled)");
            sendJson(HTTP_BADREQUEST, R"({"error":"Invalid authentication ticket"})");
            return;
        }
    }

    // The client redeems its launch ticket here to authenticate itself; remember the identity so the
    // join reads it directly (the client's persisted .LOGINSESSION cookie can't be relied on).
    mEmu->SetCurrentLaunchUser(*resolved);

    nlohmann::json user;
    user["id"] = resolved->id;
    user["name"] = resolved->name;
    user["displayName"] = resolved->displayName;
    user["isGuest"] = resolved->isGuest;

    nlohmann::json response;
    response["user"] = user;

    Out("AuthTicketRedeemHandler", "Redeemed ticket for {} (id {}){}",
        resolved->name, resolved->id, resolved->isGuest ? " [guest]" : "");
    sendJson(HTTP_OK, response.dump());
}
