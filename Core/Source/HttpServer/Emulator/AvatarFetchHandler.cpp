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
        char* end = nullptr;
        long long id = std::strtoll(v, &end, 10);
        if (end != v)
            result = static_cast<int64_t>(id);
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
    if (std::optional<int64_t> userId = ParseUserId(req)) {
        if (std::optional<std::string> federated = mEmu->GetAvatarOverride(*userId)) {
            Out("AvatarFetchHandler", "Serving federated avatar for userId={}", *userId);
            SendJson(req, *federated);
            return;
        }
    }

    nlohmann::json j = AvatarAppearance::BuildAvatarFetchJson(mEmu->GetCore());
    SendJson(req, j.dump());
}

void AvatarFetchHandler::OnRequest(evhttp_request *req, void *userdata) {
    if (mEmu->TryProxyRequest(req, [this](evhttp_request *r) { ServeLocal(r); }))
        return;
    ServeLocal(req);
}
