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
// File: PlaceUniverseHandler.cpp
// Started by: Hattozo
// Started on: 6/6/2026
// Description: Resolves a place id to the universe it belongs to (apis.roblox.com/universes/v1/places/{placeId}/universe).
#include <NoobWarrior/HttpServer/Emulator/PlaceUniverseHandler.h>
#include <NoobWarrior/HttpServer/Base/HttpServer.h>
#include <NoobWarrior/NoobWarrior.h>

#include <nlohmann/json.hpp>

#include <cstdlib>

using namespace NoobWarrior;

PlaceUniverseHandler::PlaceUniverseHandler(HttpServer *srv, EmuDbManager *dbm) :
    mHttpServer(srv),
    mEmuDbManager(dbm)
{}

void PlaceUniverseHandler::OnRequest(evhttp_request *req, void *userdata) {
    // The place id arrives as the :placeId route segment, captured by RootHandler before it
    // dispatched to us (e.g. /universes/v1/places/1818/universe -> placeId = "1818").
    const std::string placeIdStr = mHttpServer->GetRouteParam("placeId");

    char *endPtr;
    int64_t placeId = strtoll(placeIdStr.c_str(), &endPtr, 10);
    if (placeIdStr.empty() || *endPtr != '\0') {
        evhttp_send_error(req, 400, "Invalid place id");
        return;
    }

    // The real apis.roblox.com endpoint answers with the place's owning universe id. When the
    // database has no mapping for this place we fall back to a 1:1 place==universe identity so the
    // engine can still proceed (the rest of the emulated flow keys off whatever we return here).
    std::optional<int64_t> universeId = mEmuDbManager->GetUniverseIdForPlace(placeId);
    if (!universeId.has_value()) {
        mCore->Out("PlaceUniverseHandler", "No universe mapping for place {}; falling back to universeId={}", placeId, placeId);
        universeId = placeId;
    }

    nlohmann::json j;
    j["universeId"] = universeId.value();

    const std::string body = j.dump();
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* buf = evbuffer_new();
    evbuffer_add(buf, body.data(), body.size());
    evhttp_send_reply(req, HTTP_OK, nullptr, buf);
    evbuffer_free(buf);
}
