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
#include <nlohmann/json.hpp>

using namespace NoobWarrior;

GamesHandler::GamesHandler() {}

void GamesHandler::OnRequest(evhttp_request *req, void *userdata) {
    nlohmann::json game;
    game["id"] = 1;
    game["rootPlaceId"] = 1818;
    game["name"] = "noobWarrior Place";
    game["description"] = "";
    game["sourceName"] = "noobWarrior Place";
    game["sourceDescription"] = "";
    game["creator"]["id"] = 1;
    game["creator"]["name"] = "Player";
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
    game["universeAvatarType"] = "MorphToR6";
    game["genre"] = "All";
    game["isAllGenre"] = true;
    game["isFavoritedByUser"] = false;
    game["favoritedCount"] = 0;

    nlohmann::json j;
    j["data"] = nlohmann::json::array({game});

    const std::string body = j.dump();
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* buf = evbuffer_new();
    evbuffer_add_printf(buf, "%s", body.c_str());
    evhttp_send_reply(req, HTTP_OK, nullptr, buf);
    evbuffer_free(buf);
}
