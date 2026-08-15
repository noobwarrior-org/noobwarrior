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
// File: StudioOpenPlaceHandler.cpp
// Started by: Hattozo
// Started on: 5/26/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/StudioOpenPlaceHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Registry.h>
#include <NoobWarrior/Log.h>

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <string>

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

StudioOpenPlaceHandler::StudioOpenPlaceHandler(ServerEmulator *server) : mServer(server) {}

void StudioOpenPlaceHandler::OnRequest(evhttp_request *req, void *userdata) {
    const char* uri = evhttp_request_get_uri(req);
    evhttp_connection* conn = evhttp_request_get_connection(req);

    const char* peer_address = "";
    uint16_t peer_port {};
    if (conn != NULL)
        evhttp_connection_get_peer(conn, &peer_address, &peer_port);
    Out("StudioOpenPlaceHandler", "{}:{} requested {}", peer_address, peer_port, uri);
    
    char* endPtr = nullptr;
    int64_t placeId = strtoll(GetQueryParam(uri, "placeId").c_str(), &endPtr, 10);
    if (endPtr == nullptr || *endPtr != '\0' || placeId <= 0)
        placeId = 1;

    EmuDbManager* dbm = mServer->GetCore()->GetEmuDbManager();
    int64_t universeId = dbm->GetUniverseIdForPlace(placeId).value_or(placeId);
    std::string name = dbm->GetItemName(ItemType::Universe, universeId).value_or("noobWarrior Place");

    Registry* reg = mServer->GetCore()->GetRegistry();
    int64_t creatorId = dbm->GetCreatorUserId(ItemType::Universe, universeId).value_or(0);
    if (creatorId == 0)
        creatorId = reg->GetKeyValue<int64_t>("user.id").value_or(1);

    // Remember which database holds this game so its descendant-asset publishes (uploadnewasset) and
    // its place upload land in the same .nwdb, plus the place id itself so requests served while it's
    // open (see GetCurrentPlaceId) can say which game they belong to.
    EmuDb* placeDb = dbm->GetFirstDbWhereItemExists(ItemType::Asset, placeId);
    if (placeDb == nullptr)
        placeDb = dbm->GetFirstDbWhereItemExists(ItemType::Universe, universeId);
    if (placeDb != nullptr)
        mServer->SetActiveEditDbFile(placeDb->GetFileName());
    mServer->SetActiveEditPlaceId(placeId);

    nlohmann::json j;
    j["universe"] = {
        {"Id", universeId},
        {"RootPlaceId", placeId},
        {"Name", name},
        {"IsArchived", false},
        {"CreatorType", "User"},
        {"CreatorTargetId", creatorId},
        {"PrivacyType", "Public"},
        {"Created", "2013-11-01T08:47:14.07+00:00"},
        {"Updated", "2023-05-02T22:03:01.107+00:00"},
    };
    j["teamCreateEnabled"] = false;
    j["place"] = {{"Creator", {{"CreatorType", "User"}, {"CreatorTargetId", creatorId}}}};

    const std::string body = j.dump();
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* reply = evbuffer_new();
    evbuffer_add(reply, body.data(), body.size());
    evhttp_send_reply(req, 200, nullptr, reply);
    evbuffer_free(reply);
}