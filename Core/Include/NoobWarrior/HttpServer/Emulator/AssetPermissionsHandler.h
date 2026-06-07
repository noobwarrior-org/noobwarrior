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
// File: AssetPermissionsHandler.h
// Started by: Hattozo
// Started on: 6/7/2026
// Description: asset-permissions-api batch check (POST /asset-permissions-api/v1/assets/check-permissions).
//              The AudioDiscovery plugin gates every track behind a "can this universe use this audio?"
//              check when a published place (GameId != 0) is open; we grant all so the audio rows render.
#pragma once
#include <NoobWarrior/HttpServer/Base/Handler.h>

namespace NoobWarrior {
class AssetPermissionsHandler : public Handler {
public:
    AssetPermissionsHandler() = default;
    void OnRequest(evhttp_request *req, void *userdata) override;
};
}
