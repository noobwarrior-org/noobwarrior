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
// Description: HTTP request handler that retrieves place thumbnail images
#include <NoobWarrior/HttpServer/Emulator/GameIconHandler.h>
#include <NoobWarrior/NoobWarrior.h>

using namespace NoobWarrior;

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
    Out("GameIconHandler", "{}:{} requested game icon {}", peer_address, peer_port, uri);

    evkeyvalq headers;
    if (evhttp_parse_query(uri, &headers) != 0) {
        evhttp_send_error(req, 500, "Failed to parse URL parameters");
        return;
    }

    const char* idStr = evhttp_find_header(&headers, "assetId");
    if (idStr == NULL) {
        evhttp_send_error(req, 400, "assetId parameter not given");
        return;
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
    std::string contentDispositionVal;
    SqlDb::Response res = mEmuDbManager->RetrieveAssetData(id, 0, &data, &hash);
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
