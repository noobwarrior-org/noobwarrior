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
// File: GamesListHandler.cpp
// Started by: Hattozo
// Started on: 6/6/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/GamesListHandler.h>
#include <nlohmann/json.hpp>

using namespace NoobWarrior;

GamesListHandler::GamesListHandler() {}

void GamesListHandler::OnRequest(evhttp_request *req, void *userdata) {
    nlohmann::json game;
    game["creatorId"] = 1;
    game["creatorName"] = "Player";
    game["creatorType"] = "User";
    game["creatorHasVerifiedBadge"] = false;
    game["totalUpVotes"] = 1;
    game["totalDownVotes"] = 0;
    game["universeId"] = 1;
    game["name"] = "noobWarrior Server";
    game["placeId"] = 1818;
    game["playerCount"] = 1;
    game["imageToken"] = nullptr;
    game["isSponsored"] = false;
    game["nativeAdData"] = "";
    game["isShowSponsoredLabel"] = false;
    game["price"] = nullptr;
    game["analyticsIdentifier"] = nullptr;
    game["gameDescription"] = "noobWarrior team test";
    game["genre"] = "All";

    nlohmann::json j;
    j["games"] = nlohmann::json::array({game});
    j["suggestedKeyword"] = nullptr;
    j["correctedKeyword"] = nullptr;
    j["filteredKeyword"] = nullptr;
    j["hasMoreRows"] = false;
    j["nextPageExclusiveStartId"] = nullptr;
    j["featuredSearchUniverseId"] = nullptr;
    j["emphasis"] = false;
    j["cutOffIndex"] = nullptr;
    j["algorithm"] = nullptr;
    j["algorithmQueryType"] = nullptr;
    j["suggestionAlgorithm"] = nullptr;
    j["relatedGames"] = nlohmann::json::array();
    j["esDebugInfo"] = nullptr;

    const std::string body = j.dump();
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* buf = evbuffer_new();
    evbuffer_add_printf(buf, "%s", body.c_str());
    evhttp_send_reply(req, HTTP_OK, nullptr, buf);
    evbuffer_free(buf);
}
