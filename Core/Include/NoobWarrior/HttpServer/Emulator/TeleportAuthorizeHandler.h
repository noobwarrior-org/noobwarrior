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
// File: TeleportAuthorizeHandler.h
// Started by: Hattozo
// Started on: 8/24/2026
// Description: Authorizes LocalRcc teleports and starts missing destination servers.
#pragma once
#include <NoobWarrior/HttpServer/Base/Handler.h>

#include <chrono>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>

namespace NoobWarrior {
class ServerEmulator;

class TeleportAuthorizeHandler : public Handler {
public:
    explicit TeleportAuthorizeHandler(ServerEmulator *emu);
    void OnRequest(evhttp_request *req, void *userdata) override;

private:
    struct PendingLaunch {
        uint16_t Port {0};
        std::chrono::steady_clock::time_point StartedAt;
    };

    void HandleLocally(evhttp_request *req);
    bool EnsureDestinationServer(int64_t placeId, std::string *error);
    std::string CreateTeleportToken(int64_t userId, int64_t placeId);

    ServerEmulator *mEmu;
    std::mutex mPendingLaunchesMutex;
    std::map<int64_t, PendingLaunch> mPendingLaunches;
};
}
