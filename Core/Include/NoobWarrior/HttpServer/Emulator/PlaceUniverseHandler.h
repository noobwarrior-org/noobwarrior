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
// File: PlaceUniverseHandler.h
// Started by: Hattozo
// Started on: 6/6/2026
// Description: Resolves a place id to the universe it belongs to (apis.roblox.com/universes/v1/places/{placeId}/universe).
#pragma once
#include <NoobWarrior/HttpServer/Base/Handler.h>
#include <NoobWarrior/EmuDb/EmuDbManager.h>

namespace NoobWarrior {
class HttpServer;
class PlaceUniverseHandler : public Handler {
public:
    PlaceUniverseHandler(HttpServer *srv, EmuDbManager *dbm);
    void OnRequest(evhttp_request *req, void *userdata) override;
private:
    HttpServer *mHttpServer;
    EmuDbManager *mEmuDbManager;
};
}
