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
// File: AvatarFetchHandler.cpp
// Started by: Hattozo
// Started on: 6/5/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/AvatarFetchHandler.h>
#include <NoobWarrior/HttpServer/Emulator/AvatarAppearance.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Registry.h>
#include <NoobWarrior/Log.h>
#include <nlohmann/json.hpp>

#include <cstdlib>
#include <optional>
#include <string>

using namespace NoobWarrior;

namespace {
std::optional<int64_t> ParseUserId(evhttp_request *req) {
    const char* uri = evhttp_request_get_uri(req);
    if (uri == nullptr)
        return std::nullopt;
    evkeyvalq params;
    if (evhttp_parse_query(uri, &params) != 0)
        return std::nullopt;
    std::optional<int64_t> result;
    if (const char* v = evhttp_find_header(&params, "userId")) {
        // Some clients double-encode the userId, so a negative id arrives as "%2D9970" after one
        // decode. Decode again (a no-op for a plain id) before parsing.
        char* decoded = evhttp_uridecode(v, 0, nullptr);
        const char* s = decoded ? decoded : v;
        char* end = nullptr;
        long long id = std::strtoll(s, &end, 10);
        if (end != s)
            result = static_cast<int64_t>(id);
        if (decoded)
            free(decoded);
    }
    evhttp_clear_headers(&params);
    return result;
}

void SendJson(evhttp_request *req, const std::string &body) {
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* buf = evbuffer_new();
    evbuffer_add(buf, body.data(), body.size());
    evhttp_send_reply(req, HTTP_OK, nullptr, buf);
    evbuffer_free(buf);
}
}

AvatarFetchHandler::AvatarFetchHandler(ServerEmulator* emu) : mEmu(emu) {

}

void AvatarFetchHandler::ServeLocal(evhttp_request *req) {
    Core* core = mEmu->GetCore();
    std::optional<int64_t> userId = ParseUserId(req);

    // Guests (negative id) always get a plain black/white placeholder.
    if (userId && *userId < 0) {
        SendJson(req, AvatarAppearance::BuildGuestAvatarFetchJson().dump());
        return;
    }

    // Auth mode: serve the requested user's DB-stored avatar. The local registry appearance and any
    // client-pushed override are ignored, so a joiner can't spoof or leak an appearance.
    if (core->GetRegistry()->GetKeyValue<bool>("emu.auth.enabled").value_or(false)) {
        int64_t id = userId.value_or(core->GetRegistry()->GetKeyValue<int64_t>("user.id").value_or(1000));
        SendJson(req, AvatarAppearance::BuildAvatarFetchJsonForUser(core, id).dump());
        return;
    }

    // Auth off (legacy): a federated override for a remote player, else the local registry appearance.
    int64_t localUserId = core->GetRegistry()->GetKeyValue<int64_t>("user.id").value_or(1000);
    if (userId && *userId != localUserId) {
        if (std::optional<std::string> federated = mEmu->GetAvatarOverride(*userId)) {
            Out("AvatarFetchHandler", "Serving federated avatar for userId={}", *userId);
            SendJson(req, *federated);
            return;
        }
    }
    SendJson(req, AvatarAppearance::BuildAvatarFetchJson(core).dump());
}

void AvatarFetchHandler::OnRequest(evhttp_request *req, void *userdata) {
    if (mEmu->TryProxyRequest(req, [this](evhttp_request *r) { ServeLocal(r); }))
        return;
    ServeLocal(req);
}
