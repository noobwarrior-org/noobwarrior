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
// File: GameStartInfoHandler.cpp
// Started by: Hattozo
// Started on: 8/24/2026
// Description: Serves avatar.roblox.com/v1/game-start-info for local game servers.
#include <NoobWarrior/HttpServer/Emulator/GameStartInfoHandler.h>

#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/Roblox/Api/Universe.h>

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <optional>

using namespace NoobWarrior;

static Roblox::UniverseAvatarType ResolveAvatarType(EmuDbManager *dbm, int64_t requestedId) {
    std::optional<int> stored = dbm->GetUniverseAvatarType(requestedId);
    if (!stored.has_value()) {
        // Some legacy launch paths put the place id in their UniverseId slot. Accept that shape so
        // LocalRcc and dedicated RCC both enforce the universe's actual setting.
        if (std::optional<int64_t> mapped = dbm->GetUniverseIdForPlace(requestedId))
            stored = dbm->GetUniverseAvatarType(mapped.value());
    }

    if (stored.has_value() &&
        stored.value() == static_cast<int>(Roblox::UniverseAvatarType::PlayerChoice))
        return Roblox::UniverseAvatarType::PlayerChoice;
    if (stored.has_value() &&
        stored.value() == static_cast<int>(Roblox::UniverseAvatarType::MorphToR15))
        return Roblox::UniverseAvatarType::MorphToR15;
    return Roblox::UniverseAvatarType::MorphToR6;
}

static nlohmann::json AvatarScales(double height, double width, double head) {
    return {
        {"height", height},
        {"width", width},
        {"head", head},
        {"depth", 0.0},
        {"proportion", 0.0},
        {"bodyType", 0.0},
    };
}

GameStartInfoHandler::GameStartInfoHandler(ServerEmulator *serverEmulator, EmuDbManager *dbm) :
    mServerEmulator(serverEmulator),
    mEmuDbManager(dbm) {}

void GameStartInfoHandler::OnRequest(evhttp_request *req, void *userdata) {
    int64_t requestedId = 0;
    evkeyvalq query;
    const char *uri = evhttp_request_get_uri(req);
    if (uri == nullptr || evhttp_parse_query(uri, &query) != 0) {
        evhttp_send_error(req, 400, "Invalid query");
        return;
    }

    if (const char *value = evhttp_find_header(&query, "universeId")) {
        char *end = nullptr;
        requestedId = std::strtoll(value, &end, 10);
        if (value == end || *end != '\0' || requestedId <= 0)
            requestedId = 0;
    }
    evhttp_clear_headers(&query);

    // Studio's local server has historically omitted or zeroed UniverseId in a few launch modes.
    // The active place is enough to recover it through EmuDb's place-to-universe mapping.
    if (requestedId == 0)
        requestedId = mServerEmulator->GetCurrentPlaceId().value_or(0);
    if (requestedId == 0) {
        evhttp_send_error(req, 400, "Universe id not given");
        return;
    }

    const Roblox::UniverseAvatarType avatarType =
        ResolveAvatarType(mEmuDbManager, requestedId);

    // This is Roblox's v1.1 response shape. Only avatar type is currently configurable in EmuDb;
    // the remaining values are Roblox's legacy neutral defaults and deliberately stay present for
    // the 0.719 parser even though newer clients ignore several of them.
    nlohmann::json response;
    response["gameAvatarType"] = Roblox::UniverseAvatarTypeAsApiString(avatarType);
    response["allowCustomAnimations"] = "False";
    response["universeAvatarCollisionType"] = "Invalid";
    response["universeAvatarBodyType"] = "Invalid";
    response["jointPositioningType"] = "ArtistIntent";
    response["message"] = "";
    response["universeAvatarMinScales"] = AvatarScales(0.9, 0.7, 0.95);
    response["universeAvatarMaxScales"] = AvatarScales(1.05, 1.0, 1.0);
    response["universeAvatarAssetOverrides"] = nlohmann::json::array();
    response["moderationStatus"] = nullptr;

    const std::string body = response.dump();
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer *buffer = evbuffer_new();
    evbuffer_add(buffer, body.data(), body.size());
    evhttp_send_reply(req, HTTP_OK, nullptr, buffer);
    evbuffer_free(buffer);
}
