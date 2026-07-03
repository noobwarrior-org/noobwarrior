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
// File: AvatarOverrideHandler.cpp
// Started by: Hattozo
// Started on: 6/20/2026
// Description: Receiver for client-federated avatar appearances. See AvatarOverrideHandler.h.
//              Some assistance by Claude Opus 4.8
#include <NoobWarrior/HttpServer/Emulator/AvatarOverrideHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Registry.h>
#include <NoobWarrior/Log.h>

#include <nlohmann/json.hpp>

#include <string>

using namespace NoobWarrior;

static constexpr size_t kMaxOverrideBody = 256 * 1024;

AvatarOverrideHandler::AvatarOverrideHandler(ServerEmulator* emu) : mEmu(emu) {

}

void AvatarOverrideHandler::OnRequest(evhttp_request *req, void *userdata) {
    if (evhttp_request_get_command(req) != EVHTTP_REQ_POST) {
        evhttp_send_error(req, HTTP_BADMETHOD, "Request method needs to be POST");
        return;
    }

    // In auth mode the server serves avatars from its own database, so a client-pushed appearance is
    // ignored — accepting it would let a joiner spoof any user's look.
    if (mEmu->GetCore()->GetRegistry()->GetKeyValue<bool>("emu.auth.enabled").value_or(false)) {
        evhttp_send_reply(req, HTTP_OK, nullptr, nullptr);
        return;
    }

    evbuffer *buf = evhttp_request_get_input_buffer(req);
    size_t len = buf ? evbuffer_get_length(buf) : 0;
    if (len > kMaxOverrideBody) {
        evhttp_send_error(req, HTTP_BADREQUEST, "Override body too large");
        return;
    }
    std::string body(len, '\0');
    if (len > 0)
        evbuffer_copyout(buf, body.data(), len);

    try {
        nlohmann::json j = nlohmann::json::parse(body);
        if (!j.contains("userId") || !j["userId"].is_number_integer()) {
            evhttp_send_error(req, HTTP_BADREQUEST, "Missing or invalid userId");
            return;
        }
        if (!j.contains("avatarFetch") || !j["avatarFetch"].is_object()) {
            evhttp_send_error(req, HTTP_BADREQUEST, "Missing avatarFetch object");
            return;
        }

        int64_t userId = j["userId"].get<int64_t>();
        mEmu->SetAvatarOverride(userId, j["avatarFetch"].dump());

        const char *peer = "";
        uint16_t peerPort = 0;
        if (evhttp_connection *conn = evhttp_request_get_connection(req))
            evhttp_connection_get_peer(conn, &peer, &peerPort);
        Out("AvatarOverrideHandler", "Stored federated avatar for userId={} from {}", userId, peer);

        evhttp_send_reply(req, HTTP_OK, nullptr, nullptr);
    } catch (const nlohmann::json::exception &e) {
        Out("AvatarOverrideHandler", "Malformed override body: {}", e.what());
        evhttp_send_error(req, HTTP_BADREQUEST, "Malformed JSON");
    }
}
