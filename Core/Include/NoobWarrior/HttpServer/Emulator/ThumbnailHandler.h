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
// File: ThumbnailHandler.h
// Started by: Hattozo
// Started on: 6/6/2026
// Description: thumbnails.roblox.com batch endpoint (/v1/batch) plus the local image endpoint its
//              imageUrls point at (/emu-thumbnail), serving asset/user images from the databases.
#pragma once
#include <NoobWarrior/HttpServer/Base/Handler.h>
#include <NoobWarrior/EmuDb/EmuDbManager.h>

namespace NoobWarrior {
class ThumbnailHandler : public Handler {
public:
    ThumbnailHandler(EmuDbManager *dbm);
    void OnRequest(evhttp_request *req, void *userdata) override;
private:
    void ServeBatch(evhttp_request *req);
    void ServeImage(evhttp_request *req);

    EmuDbManager *mEmuDbManager;
};
}
