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
// File: EmuDbGamesHandler.cpp
// Started by: Hattozo
// Started on: 8/24/2026
// Description: Serves the list of playable games across every mounted EmuDb.
#include <NoobWarrior/HttpServer/Emulator/EmuDbGamesHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/EmuDb/EmuDbManager.h>
#include <NoobWarrior/Log.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Registry.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <string>
#include <unordered_set>
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

EmuDbGamesHandler::EmuDbGamesHandler(ServerEmulator* emu) : mEmu(emu) {

}

void EmuDbGamesHandler::OnRequest(evhttp_request *req, void *userdata) {
    const char* peerAddress = "";
    uint16_t peerPort {};
    if (evhttp_connection* conn = evhttp_request_get_connection(req))
        evhttp_connection_get_peer(conn, &peerAddress, &peerPort);
    
    std::string peer = peerAddress ? peerAddress : "";
    if (peer.empty() || !IsLoopbackOrEmpty(peer)) {
        mCore->Out("EmuDbGamesHandler", "Refused non-loopback request from {}:{}", peer, peerPort);
        evhttp_send_error(req, HTTP_NOTFOUND, "Not Found");
        return;
    }

    const char* uri = evhttp_request_get_uri(req);
    int limit = std::atoi(GetQueryParam(uri, "limit").c_str());
    if (limit <= 0) limit = 100;
    if (limit > 500) limit = 500;
    int offset = std::atoi(GetQueryParam(uri, "offset").c_str());
    if (offset < 0) offset = 0;

    EmuDbManager* dbManager = mEmu->GetCore()->GetEmuDbManager();

    std::vector<int64_t> universeIds;
    std::unordered_set<int64_t> seen;
    for (bool groupOwned : {false, true}) {
        for (int64_t id : dbManager->ListUniverseIds(groupOwned, std::numeric_limits<int>::max(), 0)) {
            if (seen.insert(id).second)
                universeIds.push_back(id);
        }
    }

    Registry* reg = mEmu->GetCore()->GetRegistry();
    int64_t selfUserId = reg != nullptr ? reg->GetKeyValue<int64_t>("user.id").value_or(1) : 1;
    std::string selfUserName = reg != nullptr
        ? reg->GetKeyValue<std::string>("user.name").value_or("Player")
        : "Player";

    nlohmann::json games = nlohmann::json::array();
    size_t skipped = 0;
    for (int64_t universeId : universeIds) {
        if (games.size() >= static_cast<size_t>(limit))
            break;

        std::vector<int64_t> placeIds = dbManager->ListUniversePlaceIds(universeId);
        if (placeIds.empty())
            continue;
        if (skipped < static_cast<size_t>(offset)) {
            skipped++;
            continue;
        }

        std::optional<EmuDb::UniverseSummary> summary = dbManager->GetUniverseSummary(universeId);

        bool groupOwned = summary && summary->GroupId && *summary->GroupId != 0;
        int64_t creatorId;
        std::string creatorName;
        if (groupOwned) {
            creatorId = *summary->GroupId;
            creatorName = dbManager->GetItemName(ItemType::Group, creatorId).value_or("Group");
        } else {
            creatorId = summary && summary->UserId && *summary->UserId != 0 ? *summary->UserId : selfUserId;
            creatorName = dbManager->GetItemName(ItemType::User, creatorId).value_or(selfUserName);
        }

        nlohmann::json game;
        game["universeId"]  = universeId;
        game["name"]        = summary && !summary->Name.empty()
            ? summary->Name
            : dbManager->GetItemName(ItemType::Asset, placeIds.front()).value_or("noobWarrior Place");
        game["rootPlaceId"] = placeIds.front();
        game["placeIds"]    = placeIds;
        game["creatorType"] = groupOwned ? "Group" : "User";
        game["creatorId"]   = creatorId;
        game["creatorName"] = creatorName;
        game["isActive"]    = summary ? summary->Active : true;
        game["created"]     = summary ? summary->Created : 0;
        game["updated"]     = summary ? summary->Updated : 0;
        game["iconUrl"]     = "/Thumbs/GameIcon.ashx?universeId=" + std::to_string(universeId);
        if (EmuDb* owner = dbManager->GetFirstDbWhereItemExists(ItemType::Universe, universeId))
            game["database"] = owner->GetFileName();
        else
            game["database"] = nullptr;

        games.push_back(std::move(game));
    }

    nlohmann::json j;
    j["games"] = std::move(games);

    const std::string body = j.dump();
    evkeyvalq* headers = evhttp_request_get_output_headers(req);
    evhttp_add_header(headers, "Content-Type", "application/json");
    evhttp_add_header(headers, "Cache-Control", "no-store");
    evbuffer* reply = evbuffer_new();
    evbuffer_add(reply, body.data(), body.size());
    evhttp_send_reply(req, HTTP_OK, nullptr, reply);
    evbuffer_free(reply);
}
