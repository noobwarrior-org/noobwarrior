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
// File: ClientSettingsV2StudioHandler.cpp
// Started by: Hattozo
// Started on: 5/18/2026
// Description: Returns a JSON object containing application settings (FFlags & DFFlags)
#include <NoobWarrior/HttpServer/Emulator/ClientSettingsV2StudioHandler.h>
#include <NoobWarrior/HttpServer/Emulator/DesktopSettingsFrame.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Log.h>

#include "FFlagJson/PCStudioAppV2.json.inc.cpp"
#include "FFlagJson/PCStudioAppV2_719.json.inc.cpp"
#include "FFlagJson/PCDesktopClientV2.dcz.inc.cpp"
#include "FFlagJson/PCDesktopClientV2.json.inc.cpp"

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <optional>
#include <string>
#include <vector>

using namespace NoobWarrior;

static int StudioEngineGeneration(const std::string& version) {
    const auto firstDot = version.find('.');
    if (firstDot == std::string::npos)
        return 0;
    return static_cast<int>(std::strtol(version.c_str() + firstDot + 1, nullptr, 10));
}

static std::optional<nlohmann::json> DecodeArchivedDesktopSettings(Core* core) {
    const std::optional<std::vector<char>> dictionary = FindDesktopSettingsDictionary(core);
    if (!dictionary)
        return std::nullopt;

    const std::optional<std::string> plain =
        DecompressWithDictionary(PCDesktopClientV2_dcz, PCDesktopClientV2_dcz_size, *dictionary);
    if (!plain)
        return std::nullopt;

    nlohmann::json parsed = nlohmann::json::parse(*plain, nullptr, false);
    if (!parsed.is_object() || !parsed.contains("applicationSettings") ||
        !parsed["applicationSettings"].is_object())
        return std::nullopt;
    return parsed;
}

static nlohmann::json GetStudioSettings(ServerEmulator* emu) {
    Core* core = emu != nullptr ? emu->GetCore() : nullptr;
    const std::string version = emu != nullptr ? emu->GetLaunchedStudioVersion().first : std::string();

    if (StudioEngineGeneration(version) < 700) { // old versions of studio get a hardcoded fflag dump from 0.574 era
        nlohmann::json legacy = nlohmann::json::parse(PCStudioAppV2_json);
        legacy["applicationSettings"]["FFlagEnableVideoPlaybackOnServer"] = "True";
        return legacy;
    }
    
    nlohmann::json settings = nlohmann::json::parse(PCStudioAppV2_719_json);

    // const std::optional<nlohmann::json> archived = DecodeArchivedDesktopSettings(core);
    // nlohmann::json settings = archived.value_or(nlohmann::json::parse(PCDesktopClientV2_json));
    
    {
        nlohmann::json& flags = settings["applicationSettings"];
        // stupid shit that needs be enabled for video to work
        flags["FFlagVideoRegisterMpegTs"] = "True";
        flags["FFlagVideoRegisterNewWebm"] = "True";
        flags["FFlagEnableVideoPlaybackOnServer"] = "True";
        // stupid shit that needs to be enabled for modern explorer to work
        flags["FFlagInstanceExtensionsServiceCountChildren"] = "True";
    }

    if (core != nullptr &&
        core->GetRegistry()->GetKeyValue<bool>("debug.log_http_server_requests").value_or(false)) {
        nlohmann::json& flags = settings["applicationSettings"];
        flags["DFLogHttpTrace"] = "7";
        flags["DFLogBatchAssetApiLog"] = "7";
    }

    emu->GetCore()->Out("ClientSettingsV2StudioHandler", "Serving Studio \"{}\" the merged 04/29/2026 14:12:10 snapshot ({} flags)", version, settings["applicationSettings"].size());
    return settings;
}

ClientSettingsV2StudioHandler::ClientSettingsV2StudioHandler(ServerEmulator* server) : mEmu(server) {}

void ClientSettingsV2StudioHandler::OnRequest(evhttp_request *req, void *userdata) {
    const char* uri = evhttp_request_get_uri(req);
    evhttp_connection* conn = evhttp_request_get_connection(req);

    const char* peer_address = "";
    uint16_t peer_port {};

    if (conn != NULL)
        evhttp_connection_get_peer(conn, &peer_address, &peer_port);
    mCore->Out("ClientSettingsV2StudioHandler", "{}:{} requested client settings {}", peer_address, peer_port, uri);

    const std::string body = GetStudioSettings(mEmu).dump();

    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* reply = evbuffer_new();
    evbuffer_add(reply, body.data(), body.size());
    evhttp_send_reply(req, 200, nullptr, reply);
    evbuffer_free(reply);
}
