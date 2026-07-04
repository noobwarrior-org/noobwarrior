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
// File: GameIconHandler.h
// Started by: Hattozo
// Started on: 4/15/2026
// Description: Serves square game-icon images straight out of the mounted databases, plus the modern
//              /v1/games/icons batch endpoint that lists each universe's icon URL for the Studio home grid.
#pragma once
#include <NoobWarrior/HttpServer/Base/Handler.h>
#include <NoobWarrior/EmuDb/EmuDbManager.h>

namespace NoobWarrior {
class HttpServer;
class GameIconHandler : public Handler {
public:
    GameIconHandler(HttpServer *srv, EmuDbManager *dbm);
    void OnRequest(evhttp_request *req, void *userdata) override;
private:
    // GET /v1/games/icons?universeIds=...: the home grid's icon list. Each entry's imageUrl points
    // back at this handler's square-icon image route.
    void ServeIconsBatch(evhttp_request *req);
    // Streams a universe's square icon PNG straight from EmuDb (RetrieveImageData), or an asset's
    // binary when called with an assetId (the legacy /Thumbs/GameIcon.ashx form).
    void ServeIconImage(evhttp_request *req);

    HttpServer *mHttpServer;
    EmuDbManager *mEmuDbManager;
};
}