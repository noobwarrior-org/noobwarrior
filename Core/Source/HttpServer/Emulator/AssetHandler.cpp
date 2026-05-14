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
// File: AssetHandler.cpp
// Started by: Hattozo
// Started on: 6/19/2025
// Description: HTTP request handler that simulates the action of getting an asset from Roblox services.
#include <NoobWarrior/HttpServer/Emulator/AssetHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/NetClient.h>
#include <NoobWarrior/NoobWarrior.h>

#include <unordered_map>

// For some reason my build of Studio retrieves the material textures over the internet for some God knows what reason
// So I have to work around it by doing this
static std::unordered_map<std::string, std::string> materials = {
    {"rbxmtl-aluminum-diffuse.dds", "aluminum/diffuse.dds"},
    {"rbxmtl-aluminum-normaldetail.dds", "aluminum/normaldetail.dds"},
    {"rbxmtl-aluminum-normal.dds", "aluminum/normal.dds"},
    {"rbxmtl-aluminum-reflection.dds", "aluminum/reflection.dds"},

    {"rbxmtl-brick-diffuse.dds", "brick/diffuse.dds"},
    {"rbxmtl-brick-normaldetail.dds", "brick/normaldetail.dds"},
    {"rbxmtl-brick-normal.dds", "brick/normal.dds"},
    {"rbxmtl-brick-reflection.dds", "brick/reflection.dds"},

    {"rbxmtl-cobblestone-diffuse.dds", "cobblestone/diffuse.dds"},
    {"rbxmtl-cobblestone-normaldetail.dds", "cobblestone/normaldetail.dds"},
    {"rbxmtl-cobblestone-normal.dds", "cobblestone/normal.dds"},
    {"rbxmtl-cobblestone-reflection.dds", "cobblestone/reflection.dds"},

    {"rbxmtl-concrete-diffuse.dds", "concrete/diffuse.dds"},
    {"rbxmtl-concrete-normaldetail.dds", "concrete/normaldetail.dds"},
    {"rbxmtl-concrete-normal.dds", "concrete/normal.dds"},
    {"rbxmtl-concrete-reflection.dds", "concrete/reflection.dds"},

    {"rbxmtl-diamondplate-diffuse.dds", "diamondplate/diffuse.dds"},
    {"rbxmtl-diamondplate-normaldetail.dds", "diamondplate/normaldetail.dds"},
    {"rbxmtl-diamondplate-normal.dds", "diamondplate/normal.dds"},
    {"rbxmtl-diamondplate-reflection.dds", "diamondplate/reflection.dds"},

    {"rbxmtl-fabric-diffuse.dds", "fabric/diffuse.dds"},
    {"rbxmtl-fabric-normaldetail.dds", "fabric/normaldetail.dds"},
    {"rbxmtl-fabric-normal.dds", "fabric/normal.dds"},
    {"rbxmtl-fabric-reflection.dds", "fabric/reflection.dds"},

    {"rbxmtl-glass-diffuse.dds", "glass/diffuse.dds"},
    {"rbxmtl-glass-normaldetail.dds", "glass/normaldetail.dds"},
    {"rbxmtl-glass-normal.dds", "glass/normal.dds"},
    {"rbxmtl-glass-reflection.dds", "glass/reflection.dds"},

    {"rbxmtl-granite-diffuse.dds", "granite/diffuse.dds"},
    {"rbxmtl-granite-normaldetail.dds", "granite/normaldetail.dds"},
    {"rbxmtl-granite-normal.dds", "granite/normal.dds"},
    {"rbxmtl-granite-reflection.dds", "granite/reflection.dds"},

    {"rbxmtl-grass-diffuse.dds", "grass/diffuse.dds"},
    {"rbxmtl-grass-normaldetail.dds", "grass/normaldetail.dds"},
    {"rbxmtl-grass-normal.dds", "grass/normal.dds"},
    {"rbxmtl-grass-reflection.dds", "grass/reflection.dds"},

    {"rbxmtl-ice-diffuse.dds", "ice/diffuse.dds"},
    {"rbxmtl-ice-normaldetail.dds", "ice/normaldetail.dds"},
    {"rbxmtl-ice-normal.dds", "ice/normal.dds"},
    {"rbxmtl-ice-reflection.dds", "ice/reflection.dds"},

    {"rbxmtl-marble-diffuse.dds", "marble/diffuse.dds"},
    {"rbxmtl-marble-normaldetail.dds", "marble/normaldetail.dds"},
    {"rbxmtl-marble-normal.dds", "marble/normal.dds"},
    {"rbxmtl-marble-reflection.dds", "marble/reflection.dds"},

    {"rbxmtl-metal-diffuse.dds", "metal/diffuse.dds"},
    {"rbxmtl-metal-normaldetail.dds", "metal/normaldetail.dds"},
    {"rbxmtl-metal-normal.dds", "metal/normal.dds"},
    {"rbxmtl-metal-reflection.dds", "metal/reflection.dds"},

    {"rbxmtl-pebble-diffuse.dds", "pebble/diffuse.dds"},
    {"rbxmtl-pebble-normaldetail.dds", "pebble/normaldetail.dds"},
    {"rbxmtl-pebble-normal.dds", "pebble/normal.dds"},
    {"rbxmtl-pebble-reflection.dds", "pebble/reflection.dds"},

    {"rbxmtl-plastic-diffuse.dds", "plastic/diffuse.dds"},
    {"rbxmtl-plastic-normaldetail.dds", "plastic/normaldetail.dds"},
    {"rbxmtl-plastic-normal.dds", "plastic/normal.dds"},

    {"rbxmtl-rust-diffuse.dds", "rust/diffuse.dds"},
    {"rbxmtl-rust-normaldetail.dds", "rust/normaldetail.dds"},
    {"rbxmtl-rust-normal.dds", "rust/normal.dds"},
    {"rbxmtl-rust-reflection.dds", "rust/reflection.dds"},

    {"rbxmtl-sand-diffuse.dds", "sand/diffuse.dds"},
    {"rbxmtl-sand-normaldetail.dds", "sand/normaldetail.dds"},
    {"rbxmtl-sand-normal.dds", "sand/normal.dds"},
    {"rbxmtl-sand-reflection.dds", "sand/reflection.dds"},

    {"rbxmtl-slate-diffuse.dds", "slate/diffuse.dds"},
    {"rbxmtl-slate-normaldetail.dds", "slate/normaldetail.dds"},
    {"rbxmtl-slate-normal.dds", "slate/normal.dds"},
    {"rbxmtl-slate-reflection.dds", "slate/reflection.dds"},

    {"rbxmtl-wood-diffuse.dds", "wood/diffuse.dds"},
    {"rbxmtl-wood-normaldetail.dds", "wood/normaldetail.dds"},
    {"rbxmtl-wood-normal.dds", "wood/normal.dds"},
    {"rbxmtl-wood-reflection.dds", "wood/reflection.dds"},

    {"rbxmtl-woodplanks-diffuse.dds", "woodplanks/diffuse.dds"},
    {"rbxmtl-woodplanks-normaldetail.dds", "woodplanks/normaldetail.dds"},
    {"rbxmtl-woodplanks-normal.dds", "woodplanks/normal.dds"},
    {"rbxmtl-woodplanks-reflection.dds", "woodplanks/reflection.dds"}
};

using namespace NoobWarrior;

AssetHandler::AssetHandler(ServerEmulator *srv, EmuDbManager *dbm) :
    mServerEmulator(srv),
    mEmuDbManager(dbm)
{}

void AssetHandler::OnRequest(evhttp_request *req, void *userdata) {
    const char* uri = evhttp_request_get_uri(req);
    evhttp_connection* conn = evhttp_request_get_connection(req);

    const char* peer_address = "";
    uint16_t peer_port {};

    if (conn != NULL)
        evhttp_connection_get_peer(conn, &peer_address, &peer_port);
    Out("AssetHandler", "{}:{} requested asset {}", peer_address, peer_port, uri);

    evkeyvalq headers;
    if (evhttp_parse_query(uri, &headers) != 0) {
        evhttp_send_error(req, 500, "Failed to parse URL parameters");
        return;
    }

    const char* idStr = evhttp_find_header(&headers, "id");
    if (idStr == NULL) {
        evhttp_send_error(req, 400, "Id parameter not given");
        return;
    }

    if (mServerEmulator->GetCurrentEngine().has_value()) {
        auto idCppStr = std::string(idStr);
        if (materials.contains(idCppStr)) {
            std::filesystem::path engineDir = mServerEmulator->GetCore()->GetEngineDirectory(*mServerEmulator->GetCurrentEngine());
            std::filesystem::path fileDir = engineDir / "PlatformContent" / "pc" / "textures" / materials[idCppStr];

            if (!std::filesystem::exists(fileDir)) {
                evhttp_send_error(req, 500, "Material file doesn't exist");
                return;
            }

            std::ifstream stream(fileDir, std::ios::binary);
            if (stream.fail()) {
                evhttp_send_error(req, 500, "Failed to open material file");
                return;
            }

            auto fileSize = std::filesystem::file_size(fileDir);
            std::vector<unsigned char> data(fileSize);
            if (!stream.read(reinterpret_cast<char*>(data.data()), fileSize)) {
                evhttp_send_error(req, 500, "Failed to read material file");
                return;
            }

            evbuffer* buf = evbuffer_new();

            evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/octet-stream");
            evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Disposition", "attachment; filename=\"material.dds\"");

            evbuffer_add(buf, data.data(), data.size());
            evhttp_send_reply(req, 200, NULL, buf);

            evbuffer_free(buf);
            return;
        }
    }

    const char* verStr = evhttp_find_header(&headers, "version");
    int ver = 0;
    if (verStr != NULL) {
        char *verEndPtr;
        ver = strtol(verStr, &verEndPtr, 10);
        if (verStr == verEndPtr) {
            /* strtol failed */
            evhttp_send_error(req, 400, "Invalid version");
            return;
        }
    }

    char *idEndPtr;
    int64_t id = strtoll(idStr, &idEndPtr, 10);
    if (idStr == idEndPtr) {
        /* strtoll failed */
        evhttp_send_error(req, 400, "Invalid ID");
        return;
    }

    std::vector<unsigned char> data;
    std::string hash;
    SqlDb::Response res = mEmuDbManager->RetrieveAssetData(id, ver, &data, &hash);
    
    if (res == SqlDb::Response::NotFound || res == SqlDb::Response::MissingBlob || (res == SqlDb::Response::Success && data.empty())) {
        std::string rbxUrl = "https://assetdelivery.roblox.com/v1/asset/?id=" + std::to_string(id);
        if (ver > 0) rbxUrl += "&version=" + std::to_string(ver);

        HttpRequest rbxReq;
        rbxReq.Url = rbxUrl;
        rbxReq.UserAgent = "Roblox/WinINet";
        if (auto *acc = mServerEmulator->GetCore()->GetRbxKeychain()->GetActiveAccount())
            rbxReq.Cookie = ".ROBLOSECURITY=" + acc->Token + ";";

        NetClient netClient;
        HttpResponse rbxRes = netClient.Fetch(rbxReq);
        if (rbxRes.Code == CURLE_OK && rbxRes.HttpStatus == 200 && !rbxRes.Body.empty()) {
            Out("AssetHandler", "Forwarded asset id={} ver={} from Roblox ({} bytes)", id, ver, rbxRes.Body.size());
            data = std::move(rbxRes.Body);
            res = SqlDb::Response::Success;
        } else {
            Out("AssetHandler", "Roblox fallback failed for id={} ver={}: curl={} http={}", id, ver,
                (int)rbxRes.Code, rbxRes.HttpStatus);
        }
    }

    std::string contentDispositionVal;
    evbuffer* buf = evbuffer_new();
    switch (res) {
    case SqlDb::Response::Success:
        if (data.empty()) {
            evhttp_send_error(req, 500, "Asset data is empty");
            break;
        }

        contentDispositionVal = std::format("attachment; filename=\"{}\"", !hash.empty() ? hash : "asset");

        evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/octet-stream");
        evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Disposition", contentDispositionVal.c_str());

        evbuffer_add(buf, data.data(), data.size());
        evhttp_send_reply(req, 200, NULL, buf);
        break;
    case SqlDb::Response::NotFound:
        evhttp_send_error(req, 404, "Asset not found");
        break;
    case SqlDb::Response::MissingBlob:
        evhttp_send_error(req, 500, "Asset blob is missing");
        break;
    default:
        evhttp_send_error(req, 500, "Failed to retrieve asset data");
        break;
    }
    evbuffer_free(buf);
}
