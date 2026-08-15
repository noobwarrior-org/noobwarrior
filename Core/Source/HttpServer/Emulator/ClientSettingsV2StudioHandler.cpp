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
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Paths.h>
#include <NoobWarrior/Log.h>

#include "FFlagJson/PCStudioAppV2.json.inc.cpp"
#include "FFlagJson/PCDesktopClientV2.dcz.inc.cpp"
#include "FFlagJson/PCDesktopClientV2.json.inc.cpp"

#include <nlohmann/json.hpp>
#include <zstd.h>

#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
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

constexpr const char* kDesktopDictionaryName =
    "881010b3c0e6682563bcca18fa7e762b3dae2e81e7b5dc72cc01aa2d26aca6b5.dict";

static std::optional<std::vector<char>> FindCompressionDictionary(Core* core) {
    if (core == nullptr)
        return std::nullopt;

    std::error_code error;
    const std::filesystem::path enginesDir = core->GetUserDataDir() / NW_PATH_ENGINES;
    if (!std::filesystem::exists(enginesDir, error))
        return std::nullopt;

    for (const auto& entry : std::filesystem::directory_iterator(enginesDir, error)) {
        if (!entry.is_directory(error))
            continue;

        const std::filesystem::path path = entry.path() / "PlatformContent" / "pc" /
            "shared_compression_dictionaries" / kDesktopDictionaryName;
        std::ifstream file(path, std::ios::binary);
        if (!file)
            continue;

        return std::vector<char>{
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>()};
    }
    return std::nullopt;
}

static std::optional<nlohmann::json> DecodeArchivedDesktopSettings(Core* core) {
    const std::optional<std::vector<char>> dictionary = FindCompressionDictionary(core);
    if (!dictionary)
        return std::nullopt;

    const unsigned long long contentSize =
        ZSTD_getFrameContentSize(PCDesktopClientV2_dcz, PCDesktopClientV2_dcz_size);
    if (contentSize == ZSTD_CONTENTSIZE_ERROR || contentSize == ZSTD_CONTENTSIZE_UNKNOWN)
        return std::nullopt;

    std::string plain(static_cast<std::size_t>(contentSize), '\0');
    ZSTD_DCtx* context = ZSTD_createDCtx();
    if (context == nullptr)
        return std::nullopt;

    const std::size_t result = ZSTD_decompress_usingDict(
        context, plain.data(), plain.size(),
        PCDesktopClientV2_dcz, PCDesktopClientV2_dcz_size,
        dictionary->data(), dictionary->size());
    ZSTD_freeDCtx(context);
    if (ZSTD_isError(result) || result != plain.size())
        return std::nullopt;

    nlohmann::json parsed = nlohmann::json::parse(plain, nullptr, false);
    if (!parsed.is_object() || !parsed.contains("applicationSettings") ||
        !parsed["applicationSettings"].is_object())
        return std::nullopt;
    return parsed;
}

static nlohmann::json GetStudioSettings(ServerEmulator* emu) {
    Core* core = emu != nullptr ? emu->GetCore() : nullptr;
    const std::string version = emu != nullptr ? emu->GetLaunchedStudioVersion().first : std::string();

    if (StudioEngineGeneration(version) < 700) // old versions of studio get a hardcoded fflag dump from 0.574 era
        return nlohmann::json::parse(PCStudioAppV2_json);

    const std::optional<nlohmann::json> archived = DecodeArchivedDesktopSettings(core);
    nlohmann::json settings = archived.value_or(nlohmann::json::parse(PCDesktopClientV2_json));
    
    if (core != nullptr &&
        core->GetRegistry()->GetKeyValue<bool>("debug.log_http_server_requests").value_or(false)) {
        nlohmann::json& flags = settings["applicationSettings"];
        flags["DFLogHttpTrace"] = "7";
        flags["DFLogBatchAssetApiLog"] = "7";
    }

    Out("ClientSettingsV2StudioHandler", "Serving Studio \"{}\" the {} desktop snapshot ({} flags)",
        version, archived.has_value() ? "archived frame" : "loose JSON fallback",
        settings["applicationSettings"].size());
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
    Out("ClientSettingsV2StudioHandler", "{}:{} requested client settings {}", peer_address, peer_port, uri);

    const std::string body = GetStudioSettings(mEmu).dump();

    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* reply = evbuffer_new();
    evbuffer_add(reply, body.data(), body.size());
    evhttp_send_reply(req, 200, nullptr, reply);
    evbuffer_free(reply);
}
