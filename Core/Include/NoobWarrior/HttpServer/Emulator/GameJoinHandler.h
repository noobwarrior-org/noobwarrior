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
// File: GameJoinHandler.h
// Started by: Hattozo
// Started on: 5/26/2026
// Description:
#pragma once
#include <NoobWarrior/HttpServer/Base/Handler.h>

#include <nlohmann/json_fwd.hpp>

namespace NoobWarrior {
class ServerEmulator;
class GameJoinHandler : public Handler {
public:
    GameJoinHandler(ServerEmulator* emu);
    void OnRequest(evhttp_request *req, void *userdata) override;
private:
    void HandleLocally(evhttp_request *req);

    // Overlays this client's local player identity (user.id / user.name / user.display_name) onto a
    // join script's player fields. A remote request also carries this identity to the host before
    // the response is built; this response overlay remains as compatibility with older hosts.
    // Used only on the non-authenticated path; under auth the host is authoritative for identity.
    void ApplyLocalIdentity(nlohmann::json &joinScript);

    // Stamps a concrete player identity (from a resolved account or guest) onto a join script.
    void StampIdentity(nlohmann::json &joinScript, int64_t userId, const std::string &name,
                       const std::string &displayName);

    ServerEmulator* mEmu;
};
}
