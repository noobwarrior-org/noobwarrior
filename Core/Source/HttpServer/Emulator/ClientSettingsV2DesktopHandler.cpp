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
#include <NoobWarrior/HttpServer/Emulator/DesktopSettingsFrame.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/Log.h>

#include <event2/buffer.h>
#include <event2/http.h>
#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <vector>

#include "FFlagJson/PCDesktopClientV2.dcz.inc.cpp"
#include "FFlagJson/PCDesktopClientV2.json.inc.cpp"

using namespace NoobWarrior;

static void ApplyCustomFlags(nlohmann::json& settings) {
    if (!settings.is_object() || !settings.contains("applicationSettings"))
        return;
    nlohmann::json& flags = settings["applicationSettings"];
    if (!flags.is_object())
        return;
    flags["FFlagVideoRegisterMpegTs"] = "True";
    flags["FFlagVideoRegisterNewWebm"] = "True";
    flags["FFlagEnableVideoPlaybackOnServer"] = "True";
}

static nlohmann::json GetLocalDesktopSettings() {
    nlohmann::json settings = nlohmann::json::parse(PCDesktopClientV2_json);
    ApplyCustomFlags(settings);
    return settings;
}

static const std::string* GetPatchedCompressedSettings(Core* core) {
    static std::optional<std::string> cached;
    static bool attempted = false;
    if (attempted)
        return cached ? &*cached : nullptr;
    attempted = true;

    const std::optional<std::vector<char>> dictionary = FindDesktopSettingsDictionary(core);
    if (!dictionary) {
        Out("ClientSettingsV2DesktopHandler",
            "No compression dictionary found; serving the stock settings frame without video flags");
        return nullptr;
    }

    const std::optional<std::string> plain =
        DecompressWithDictionary(PCDesktopClientV2_dcz, PCDesktopClientV2_dcz_size, *dictionary);
    if (!plain)
        return nullptr;

    nlohmann::json settings = nlohmann::json::parse(*plain, nullptr, false);
    if (settings.is_discarded())
        return nullptr;
    ApplyCustomFlags(settings);

    std::optional<std::string> rebuilt = CompressWithDictionary(settings.dump(), *dictionary);
    if (!rebuilt)
        return nullptr;

    cached = std::move(*rebuilt);
    Out("ClientSettingsV2DesktopHandler",
        "Rebuilt the settings frame with video flags ({} flags, {} bytes)",
        settings["applicationSettings"].size(), cached->size());
    return &*cached;
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

        const std::string *patched =
            GetPatchedCompressedSettings(mEmu != nullptr ? mEmu->GetCore() : nullptr);

        evbuffer *buf = evbuffer_new();
        if (patched != nullptr)
            evbuffer_add(buf, patched->data(), patched->size());
        else
            evbuffer_add(buf, PCDesktopClientV2_dcz, PCDesktopClientV2_dcz_size);
        evhttp_send_reply(req, HTTP_OK, nullptr, buf);
        evbuffer_free(buf);
        Out("ClientSettingsV2DesktopHandler",
            "Sent {} PCDesktopClientV2 settings frame ({} bytes)",
            patched != nullptr ? "patched" : "archived 2026-05-06",
            patched != nullptr ? patched->size() : PCDesktopClientV2_dcz_size);
        return;
    }

    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* buf = evbuffer_new();
    evbuffer_add(buf, body.data(), body.size());
    evhttp_send_reply(req, HTTP_OK, nullptr, buf);
    evbuffer_free(buf);
}
