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
// File: GamesHandler.cpp
// Started by: Hattozo
// Started on: 6/6/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/GamesHandler.h>
#include <NoobWarrior/Roblox/Api/Universe.h>
#include <nlohmann/json.hpp>

#include <cstdlib>
#include <optional>
#include <string>
#include <vector>

using namespace NoobWarrior;

GamesHandler::GamesHandler(EmuDbManager *dbm) : mEmuDbManager(dbm) {}

// Builds the per-game object for /v1/games, drawing the universe's root place, name and creator
// from the mounted databases and falling back to neutral defaults when the universe isn't stored.
static nlohmann::json BuildGame(EmuDbManager *dbm, int64_t universeId) {
    int64_t rootPlaceId = dbm->GetStartPlaceIdForUniverse(universeId).value_or(universeId);
    std::string name = dbm->GetItemName(ItemType::Universe, universeId).value_or("noobWarrior Place");

    int64_t creatorId = dbm->GetCreatorUserId(ItemType::Universe, universeId).value_or(1);
    std::string creatorName = dbm->GetItemName(ItemType::User, creatorId).value_or("Player");

    nlohmann::json game;
    game["id"] = universeId;
    game["rootPlaceId"] = rootPlaceId;
    game["name"] = name;
    game["description"] = "";
    game["sourceName"] = name;
    game["sourceDescription"] = "";
    game["creator"]["id"] = creatorId;
    game["creator"]["name"] = creatorName;
    game["creator"]["type"] = "User";
    game["creator"]["isRNVAccount"] = false;
    game["creator"]["hasVerifiedBadge"] = false;
    game["price"] = nullptr;
    game["allowedGearGenres"] = nlohmann::json::array({"All"});
    game["allowedGearCategories"] = nlohmann::json::array();
    game["isGenreEnforced"] = false;
    game["copyingAllowed"] = false;
    game["playing"] = 1;
    game["visits"] = 1;
    game["maxPlayers"] = 50;
    game["created"] = "2015-01-01T00:00:00Z";
    game["updated"] = "2015-01-01T00:00:00Z";
    game["studioAccessToApisAllowed"] = true;
    game["createVipServersAllowed"] = false;
    Roblox::UniverseAvatarType avatarType = Roblox::UniverseAvatarType::MorphToR6;
    if (const std::optional<int> stored = dbm->GetUniverseAvatarType(universeId)) {
        if (*stored == static_cast<int>(Roblox::UniverseAvatarType::PlayerChoice))
            avatarType = Roblox::UniverseAvatarType::PlayerChoice;
        else if (*stored == static_cast<int>(Roblox::UniverseAvatarType::MorphToR15))
            avatarType = Roblox::UniverseAvatarType::MorphToR15;
    }
    game["universeAvatarType"] = Roblox::UniverseAvatarTypeAsApiString(avatarType);
    game["genre"] = "All";
    game["isAllGenre"] = true;
    game["isFavoritedByUser"] = false;
    game["favoritedCount"] = 0;
    return game;
}

void GamesHandler::OnRequest(evhttp_request *req, void *userdata) {
    // Roblox asks for one or more universes at once: /v1/games?universeIds=1818,1819
    std::vector<int64_t> universeIds;
    const char* uri = evhttp_request_get_uri(req);
    evkeyvalq headers;
    if (uri != nullptr && evhttp_parse_query(uri, &headers) == 0) {
        if (const char* idsStr = evhttp_find_header(&headers, "universeIds")) {
            std::string ids = idsStr;
            size_t start = 0;
            while (start <= ids.size()) {
                size_t comma = ids.find(',', start);
                std::string token = ids.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
                if (!token.empty()) {
                    char* endPtr;
                    int64_t id = strtoll(token.c_str(), &endPtr, 10);
                    if (*endPtr == '\0')
                        universeIds.push_back(id);
                }
                if (comma == std::string::npos) break;
                start = comma + 1;
            }
        }
        evhttp_clear_headers(&headers);
    }

    // No (or unparsable) universeIds: keep returning a single placeholder game so older callers
    // that hit /v1/games bare don't get an empty list.
    if (universeIds.empty())
        universeIds.push_back(1);

    nlohmann::json data = nlohmann::json::array();
    for (int64_t universeId : universeIds)
        data.push_back(BuildGame(mEmuDbManager, universeId));

    nlohmann::json j;
    j["data"] = std::move(data);

    const std::string body = j.dump();
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* buf = evbuffer_new();
    evbuffer_add(buf, body.data(), body.size());
    evhttp_send_reply(req, HTTP_OK, nullptr, buf);
    evbuffer_free(buf);
}
