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
// File: IdePublishHandler.cpp
// Started by: Hattozo
// Started on: 6/22/2026
// Description: publishes a place's descendant assets (like unions and stuff)
#include <NoobWarrior/HttpServer/Emulator/IdePublishHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Registry.h>

#include "../../algorithm/gzip.h"

#include <cctype>
#include <cstdlib>
#include <random>
#include <string>
#include <unordered_map>
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

static std::string ReadBody(evhttp_request *req) {
    std::string body;
    if (evbuffer* in = evhttp_request_get_input_buffer(req)) {
        size_t len = evbuffer_get_length(in);
        body.resize(len);
        if (len > 0)
            evbuffer_copyout(in, body.data(), len);
    }
    GunzipIfNeeded(body);
    return body;
}

static int AssetTypeIdFromName(std::string name) {
    for (char &c : name) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    static const std::unordered_map<std::string, int> map = {
        {"image", static_cast<int>(Roblox::AssetType::Image)},
        {"decal", static_cast<int>(Roblox::AssetType::Decal)},
        {"audio", static_cast<int>(Roblox::AssetType::Audio)},
        {"mesh", static_cast<int>(Roblox::AssetType::Mesh)},
        {"meshpart", static_cast<int>(Roblox::AssetType::MeshPart)},
        {"model", static_cast<int>(Roblox::AssetType::Model)},
        {"animation", static_cast<int>(Roblox::AssetType::Animation)},
        {"lua", static_cast<int>(Roblox::AssetType::Lua)},
        {"video", static_cast<int>(Roblox::AssetType::Video)},
        {"solidmodel", static_cast<int>(Roblox::AssetType::SolidModel)},
    };
    auto it = map.find(name);
    return it != map.end() ? it->second : static_cast<int>(Roblox::AssetType::None);
}

// A positive asset id not already used by any mounted database.
static int64_t GenerateUnusedAssetId(EmuDbManager *dbm) {
    static thread_local std::mt19937_64 gen{std::random_device{}()};
    std::uniform_int_distribution<int64_t> dist(0, 900000000000000LL);
    for (int i = 0; i < 16; i++) {
        int64_t id = dist(gen);
        if (dbm->GetFirstDbWhereItemExists(ItemType::Asset, id) == nullptr)
            return id;
    }
    return dist(gen); // collision after 16 tries is astronomically unlikely
}

static void ReplyId(evhttp_request *req, int64_t id) {
    const std::string out = std::to_string(id);
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "text/plain");
    evbuffer* buf = evbuffer_new();
    evbuffer_add(buf, out.data(), out.size());
    evhttp_send_reply(req, HTTP_OK, nullptr, buf);
    evbuffer_free(buf);
}

IdePublishHandler::IdePublishHandler(ServerEmulator *server) : mServer(server) {}

void IdePublishHandler::OnRequest(evhttp_request *req, void *userdata) {
    const char* uri = evhttp_request_get_uri(req);
    std::string path = uri ? uri : "";
    if (auto q = path.find('?'); q != std::string::npos)
        path.resize(q);
    for (char &c : path)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (path.find("uploadexistingasset") != std::string::npos)
        HandleUploadExistingAsset(req);
    else
        HandleUploadNewAsset(req);
}

void IdePublishHandler::HandleUploadNewAsset(evhttp_request *req) {
    const char* uri = evhttp_request_get_uri(req);
    std::string body = ReadBody(req);
    if (body.empty()) {
        evhttp_send_error(req, 400, "Empty asset upload");
        return;
    }

    EmuDbManager* dbm = mServer->GetCore()->GetEmuDbManager();

    // Publish into the same database as the game currently being edited (set when its place was
    // opened), unless it refuses writes, in which case the master database takes it and shadows it.
    EmuDb* db = nullptr;
    std::string activeDb = mServer->GetActiveEditDbFile();
    if (!activeDb.empty())
        db = dbm->GetDbFromFileName(activeDb);
    if (db != nullptr && !db->AllowsRuntimeWrites())
        db = nullptr;
    if (db == nullptr)
        db = dbm->GetMasterDatabase();
    if (db != nullptr && !db->AllowsRuntimeWrites())
        db = nullptr;
    if (db == nullptr) {
        evhttp_send_error(req, 500, "No database to publish to");
        return;
    }

    int64_t assetId = GenerateUnusedAssetId(dbm);
    std::string name = GetQueryParam(uri, "name");
    std::string typeName = GetQueryParam(uri, "assetTypeName");
    if (name.empty())
        name = typeName.empty() ? std::string("Published Asset") : typeName;
    int64_t creatorId = mServer->GetCore()->GetRegistry()->GetKeyValue<int64_t>("user.id").value_or(1);

    SqlRow row;
    row.push_back({"Id", assetId});
    row.push_back({"Name", name});
    row.push_back({"Type", AssetTypeIdFromName(typeName)});
    row.push_back({"UserId", creatorId});
    if (db->AddItem(ItemType::Asset, row) != SqlDb::Response::Success) {
        mCore->Out("IdePublishHandler", "Failed to create asset row id={} in \"{}\"", assetId, db->GetFileName());
        evhttp_send_error(req, 500, "Failed to create asset");
        return;
    }

    std::vector<unsigned char> data(body.begin(), body.end());
    if (db->AttachDataToAsset(assetId, 0, data) != SqlDb::Response::Success) {
        evhttp_send_error(req, 500, "Failed to store asset data");
        return;
    }
    db->MarkDirty();
    mCore->Out("IdePublishHandler", "Published new {} asset id={} into \"{}\"",
        typeName.empty() ? "descendant" : typeName, assetId, db->GetFileName());

    ReplyId(req, assetId);
}

void IdePublishHandler::HandleUploadExistingAsset(evhttp_request *req) {
    const char* uri = evhttp_request_get_uri(req);
    std::string idStr = GetQueryParam(uri, "assetID");
    if (idStr.empty()) idStr = GetQueryParam(uri, "assetId");
    if (idStr.empty()) idStr = GetQueryParam(uri, "assetid");
    char* endPtr = nullptr;
    int64_t assetId = strtoll(idStr.c_str(), &endPtr, 10);
    if (idStr.empty() || endPtr == idStr.c_str() || *endPtr != '\0' || assetId <= 0) {
        evhttp_send_error(req, 400, "Missing or invalid assetID");
        return;
    }

    std::string body = ReadBody(req);
    if (body.empty()) {
        evhttp_send_error(req, 400, "Empty asset upload");
        return;
    }

    EmuDbManager* dbm = mServer->GetCore()->GetEmuDbManager();
    EmuDb* db = dbm->GetFirstDbWhereItemExists(ItemType::Asset, assetId);
    if (db != nullptr && !db->AllowsRuntimeWrites())
        db = nullptr; // shipped read-only: overlay the new version from the master database instead
    if (db == nullptr) {
        std::string activeDb = mServer->GetActiveEditDbFile();
        if (!activeDb.empty())
            db = dbm->GetDbFromFileName(activeDb);
        if (db != nullptr && !db->AllowsRuntimeWrites())
            db = nullptr;
    }
    if (db == nullptr)
        db = dbm->GetMasterDatabase();
    if (db != nullptr && !db->AllowsRuntimeWrites())
        db = nullptr;
    if (db == nullptr) {
        evhttp_send_error(req, 500, "No database to publish to");
        return;
    }

    if (!db->DoesItemExist(ItemType::Asset, assetId)) {
        SqlRow row;
        row.push_back({"Id", assetId});
        row.push_back({"Name", std::string("Published Asset")});
        row.push_back({"Type", 0});
        db->AddItem(ItemType::Asset, row);
    }

    std::vector<unsigned char> data(body.begin(), body.end());
    if (db->AttachDataToAsset(assetId, 0, data) != SqlDb::Response::Success) {
        evhttp_send_error(req, 500, "Failed to store asset data");
        return;
    }
    db->MarkDirty();
    mCore->Out("IdePublishHandler", "Published new version of asset id={} into \"{}\"", assetId, db->GetFileName());

    ReplyId(req, assetId);
}
