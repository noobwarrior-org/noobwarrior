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
// File: GameIconHandler.cpp
// Started by: Hattozo
// Started on: 4/15/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/GameIconHandler.h>
#include <NoobWarrior/NoobWarrior.h>

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <format>
#include <string>
#include <vector>

using namespace NoobWarrior;

static std::string GetQueryParam(const char* uri, const char* key) {
    std::string out;
    if (uri == nullptr) return out;
    evkeyvalq query;
    if (evhttp_parse_query(uri, &query) == 0) {
        if (const char* val = evhttp_find_header(&query, key))
            out = val;
        evhttp_clear_headers(&query);
    }
    return out;
}

GameIconHandler::GameIconHandler(HttpServer *srv, EmuDbManager *dbm) :
    mHttpServer(srv),
    mEmuDbManager(dbm)
{}

void GameIconHandler::OnRequest(evhttp_request *req, void *userdata) {
    const char* uri = evhttp_request_get_uri(req);
    evhttp_connection* conn = evhttp_request_get_connection(req);

    const char* peer_address = "";
    uint16_t peer_port {};
    if (conn != NULL)
        evhttp_connection_get_peer(conn, &peer_address, &peer_port);
    Out("GameIconHandler", "{}:{} requested game icon {}", peer_address, peer_port, uri ? uri : "");

    std::string path = uri ? uri : "";
    if (auto q = path.find('?'); q != std::string::npos)
        path.resize(q);
    for (char &c : path)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (path.find("games/icons") != std::string::npos)
        ServeIconsBatch(req);
    else
        ServeIconImage(req);
}

void GameIconHandler::ServeIconsBatch(evhttp_request *req) {
    const char* uri = evhttp_request_get_uri(req);
    // universeIds arrives comma-separated: ?universeIds=1,2,3&size=150x150&format=png&returnPolicy=...
    std::string idsParam = GetQueryParam(uri, "universeIds");

    nlohmann::json data = nlohmann::json::array();
    size_t start = 0;
    while (start <= idsParam.size()) {
        size_t comma = idsParam.find(',', start);
        std::string tok = idsParam.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
        char* endPtr = nullptr;
        int64_t id = strtoll(tok.c_str(), &endPtr, 10);
        if (!tok.empty() && endPtr && *endPtr == '\0') {
            // imageUrl points back at this handler's square-icon route on an rbxcdn host (Studio only
            // accepts image urls on rbxcdn; the connect-hook redirects the host back to us). The icon
            // bytes come straight from EmuDb when that url is fetched — no extra service in between.
            nlohmann::json entry;
            entry["targetId"] = id;
            entry["state"] = "Completed";
            entry["imageUrl"] = "https://t0.rbxcdn.com/Thumbs/GameIcon.ashx?universeId=" + std::to_string(id);
            entry["errorCode"] = 0;
            entry["errorMessage"] = "";
            entry["version"] = "TN3.5";
            data.push_back(std::move(entry));
        }
        if (comma == std::string::npos) break;
        start = comma + 1;
    }

    nlohmann::json j;
    j["data"] = std::move(data);

    const std::string body = j.dump();
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* buf = evbuffer_new();
    evbuffer_add(buf, body.data(), body.size());
    evhttp_send_reply(req, 200, nullptr, buf);
    evbuffer_free(buf);
}

void GameIconHandler::ServeIconImage(evhttp_request *req) {
    const char* uri = evhttp_request_get_uri(req);

    // Modern home grid: ?universeId=<id> -> the universe's square icon, pulled directly from EmuDb
    // (RetrieveImageData chases Universe -> start place -> Asset.ImageId -> stored thumbnail blob).
    std::string universeStr = GetQueryParam(uri, "universeId");
    if (!universeStr.empty()) {
        char* endPtr = nullptr;
        int64_t universeId = strtoll(universeStr.c_str(), &endPtr, 10);
        if (endPtr == universeStr.c_str() || *endPtr != '\0') {
            evhttp_send_error(req, 400, "Invalid universe id");
            return;
        }
        std::vector<unsigned char> image = mEmuDbManager->RetrieveImageData(ItemType::Universe, universeId);
        if (image.empty()) {
            evhttp_send_error(req, 404, "No icon");
            return;
        }
        evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "image/png");
        evbuffer* buf = evbuffer_new();
        evbuffer_add(buf, image.data(), image.size());
        evhttp_send_reply(req, 200, nullptr, buf);
        evbuffer_free(buf);
        return;
    }

    // Legacy /Thumbs/GameIcon.ashx?assetId=<id> -> the asset binary itself.
    std::string idStr = GetQueryParam(uri, "assetId");
    char *idEndPtr = nullptr;
    int64_t id = strtoll(idStr.c_str(), &idEndPtr, 10);
    if (idStr.empty() || idStr.c_str() == idEndPtr || *idEndPtr != '\0') {
        evhttp_send_error(req, 400, "assetId parameter not given");
        return;
    }

    std::vector<unsigned char> data;
    std::string hash;
    SqlDb::Response res = mEmuDbManager->RetrieveAssetData(id, 0, &data, &hash);
    evbuffer* buf = evbuffer_new();
    switch (res) {
    case SqlDb::Response::Success:
        if (data.empty()) {
            evhttp_send_error(req, 500, "Asset data is empty");
            break;
        }
        evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/octet-stream");
        {
            std::string cd = std::format("attachment; filename=\"{}\"", !hash.empty() ? hash : "asset");
            evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Disposition", cd.c_str());
        }
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
