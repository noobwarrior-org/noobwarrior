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
// File: DataUploadHandler.cpp
// Started by: Hattozo
// Started on: 6/22/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/DataUploadHandler.h>
#include <NoobWarrior/NoobWarrior.h>

#include <zlib.h>

#include <cstdlib>
#include <cstring>
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

static bool Inflate(const std::string& in, std::string& out) {
    if (in.empty()) return false;
    z_stream zs{};
    if (inflateInit2(&zs, 15 + 32) != Z_OK) // auto-detect gzip or zlib header
        return false;
    zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(in.data()));
    zs.avail_in = static_cast<uInt>(in.size());

    char buf[8192];
    int ret;
    do {
        zs.next_out = reinterpret_cast<Bytef*>(buf);
        zs.avail_out = sizeof(buf);
        ret = inflate(&zs, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END && ret != Z_BUF_ERROR) {
            inflateEnd(&zs);
            return false;
        }
        out.append(buf, sizeof(buf) - zs.avail_out);
    } while (ret == Z_OK);

    inflateEnd(&zs);
    return ret == Z_STREAM_END;
}

DataUploadHandler::DataUploadHandler(EmuDbManager *dbm) : mEmuDbManager(dbm) {}

void DataUploadHandler::OnRequest(evhttp_request *req, void *userdata) {
    const char* uri = evhttp_request_get_uri(req);
    
    std::string idStr = GetQueryParam(uri, "assetid");
    if (idStr.empty())
        idStr = GetQueryParam(uri, "assetId");
    char* endPtr = nullptr;
    int64_t assetId = strtoll(idStr.c_str(), &endPtr, 10);
    if (idStr.empty() || endPtr == idStr.c_str() || *endPtr != '\0' || assetId <= 0) {
        evhttp_send_error(req, 400, "Missing or invalid assetid");
        return;
    }

    std::string body;
    if (evbuffer* in = evhttp_request_get_input_buffer(req)) {
        size_t len = evbuffer_get_length(in);
        body.resize(len);
        if (len > 0)
            evbuffer_copyout(in, body.data(), len);
    }
    if (body.empty()) {
        evhttp_send_error(req, 400, "Empty place upload");
        return;
    }

    const char* enc = evhttp_find_header(evhttp_request_get_input_headers(req), "Content-Encoding");
    bool looksGzip = (body.size() >= 2 &&
                      static_cast<unsigned char>(body[0]) == 0x1f &&
                      static_cast<unsigned char>(body[1]) == 0x8b);
    if ((enc && std::strstr(enc, "gzip")) || looksGzip) {
        std::string inflated;
        if (Inflate(body, inflated))
            body = std::move(inflated);
    }

    EmuDb* db = mEmuDbManager->GetFirstDbWhereItemExists(ItemType::Asset, assetId);
    if (db == nullptr)
        db = mEmuDbManager->GetMasterDatabase();
    if (db == nullptr) {
        evhttp_send_error(req, 500, "No database to publish to");
        return;
    }

    if (!db->DoesItemExist(ItemType::Asset, assetId)) {
        SqlRow row;
        row.push_back({"Id", assetId});
        row.push_back({"Name", std::string("Published Place")});
        row.push_back({"Type", 9});
        db->AddItem(ItemType::Asset, row);
    }

    std::vector<unsigned char> data(body.begin(), body.end());
    SqlDb::Response res = db->AttachDataToAsset(assetId, 0, data);
    if (res != SqlDb::Response::Success) {
        Out("DataUploadHandler", "Failed to publish place id={} (code={})", assetId, static_cast<int>(res));
        evhttp_send_error(req, 500, "Failed to store published place");
        return;
    }
    db->MarkDirty();
    Out("DataUploadHandler", "Published place id={} as a new version into \"{}\"", assetId, db->GetFileName());
    
    const std::string out = std::to_string(assetId);
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "text/plain");
    evbuffer* buf = evbuffer_new();
    evbuffer_add(buf, out.data(), out.size());
    evhttp_send_reply(req, HTTP_OK, nullptr, buf);
    evbuffer_free(buf);
}
