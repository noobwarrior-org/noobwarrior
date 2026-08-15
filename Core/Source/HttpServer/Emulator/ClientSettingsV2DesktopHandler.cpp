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
// File: ClientSettingsV2DesktopHandler.cpp
// Started by: Hattozo
// Started on: 6/6/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/ClientSettingsV2DesktopHandler.h>
#include <NoobWarrior/Log.h>

#include <event2/buffer.h>
#include <event2/http.h>
#include <nlohmann/json.hpp>

#include <string>

#include "FFlagJson/PCDesktopClientV2.dcz.inc.cpp"
#include "FFlagJson/PCDesktopClientV2.json.inc.cpp"

using namespace NoobWarrior;

static nlohmann::json GetLocalDesktopSettings() {
    nlohmann::json settings = nlohmann::json::parse(PCDesktopClientV2_json);
    return settings;
}

static bool IsCompressedSettingsRequest(const std::string& uri) {
    return uri.starts_with("/v2/settings-compressed/application/PCDesktopClient/") ||
           uri == "/v2/settings-compressed/application/PCDesktopClient.zst";
}

ClientSettingsV2DesktopHandler::ClientSettingsV2DesktopHandler(ServerEmulator* server) : mEmu(server) {}

void ClientSettingsV2DesktopHandler::OnRequest(evhttp_request *req, void *userdata) {
    const char *rawUri = evhttp_request_get_uri(req);
    const std::string uri = rawUri ? rawUri : "";
    const std::string body = GetLocalDesktopSettings().dump();

    if (IsCompressedSettingsRequest(uri)) {
        evkeyvalq *headers = evhttp_request_get_output_headers(req);
        evhttp_add_header(headers, "Content-Type", "application/octet-stream");
        evhttp_add_header(headers, "Cache-Control", "no-store");

        evbuffer *buf = evbuffer_new();
        evbuffer_add(buf, PCDesktopClientV2_dcz, PCDesktopClientV2_dcz_size);
        evhttp_send_reply(req, HTTP_OK, nullptr, buf);
        evbuffer_free(buf);
        Out("ClientSettingsV2DesktopHandler",
            "Sent archived 2026-05-06 PCDesktopClientV2 settings frame ({} bytes)",
            PCDesktopClientV2_dcz_size);
        return;
    }

    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* buf = evbuffer_new();
    evbuffer_add(buf, body.data(), body.size());
    evhttp_send_reply(req, HTTP_OK, nullptr, buf);
    evbuffer_free(buf);
}
