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
#include <NoobWarrior/NoobWarrior.h>

using namespace NoobWarrior;

AssetHandler::AssetHandler(HttpServer *srv, EmuDbManager *dbm) :
    mHttpServer(srv),
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
    if (mEmuDbManager->GetMasterDatabase() == nullptr) {
        evhttp_send_error(req, 500, "Master database does not exist.");
        return;
    }

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

    std::string stmtStr = "SELECT DataHash FROM AssetData WHERE Id = ?";
    if (verStr != NULL && ver > 0)
        stmtStr += " AND Version = ?;";
    else
        stmtStr += " ORDER BY Version DESC LIMIT 1;";

    char *idEndPtr;
    int64_t id = strtoll(idStr, &idEndPtr, 10);
    if (idStr == idEndPtr) {
        /* strtoll failed */
        evhttp_send_error(req, 400, "Invalid ID");
        return;
    }

    Statement getHashStmt = mEmuDbManager->GetMasterDatabase()->PrepareStatement(stmtStr);
    if (getHashStmt.Fail()) {
        evhttp_send_error(req, 500, "failed to prepare SQL statement");
        return;
    }
    getHashStmt.Bind(1, id);
    if (verStr != NULL) {
        getHashStmt.Bind(2, ver);
    }
    int res = getHashStmt.Step();
    if (res != SQLITE_ROW && res != SQLITE_DONE) {
        evhttp_send_error(req, 500, "failed to execute SQL statement");
        return;
    }
    if (res == SQLITE_ROW) {
        std::string hash = getHashStmt.GetStringFromColumnIndex(0);
        Statement blobStmt = mEmuDbManager->GetMasterDatabase()->PrepareStatement("SELECT Blob FROM BlobStorage WHERE Hash = ?");
        if (blobStmt.Fail()) {
            evhttp_send_error(req, 500, "failed to prepare SQL statement");
            return;
        }
        blobStmt.Bind(1, hash);
        int blobStmtRes = blobStmt.Step();
        if (blobStmtRes != SQLITE_ROW && blobStmtRes != SQLITE_DONE) {
            evhttp_send_error(req, 500, "failed to execute SQL statement");
            return;
        }
        if (blobStmtRes == SQLITE_ROW) {
            std::vector<unsigned char> data = blobStmt.GetBlobFromColumnIndex(0);
            evbuffer* buf = evbuffer_new();
            evbuffer_add(buf, data.data(), data.size());

            evhttp_send_reply(req, 200, NULL, buf);
            evbuffer_free(buf);
            return;
        }
        evhttp_send_error(req, 400, "Blob was not found in database");
        return;
    }
    evhttp_send_error(req, 400, "Data for ID was not found in database");
}
