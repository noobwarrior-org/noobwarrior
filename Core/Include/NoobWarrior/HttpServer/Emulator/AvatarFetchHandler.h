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
// File: AvatarFetchHandler.h
// Started by: Hattozo
// Started on: 6/1/2026
// Description:
#pragma once
#include <NoobWarrior/HttpServer/Base/Handler.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <set>

namespace NoobWarrior {
class ServerEmulator;
class AvatarFetchHandler : public Handler {
public:
    AvatarFetchHandler(ServerEmulator* emu);
    void OnRequest(evhttp_request *req, void *userdata) override;

    // Must run before the server's evhttp is freed; see AssetHandler::PauseProxy.
    void PauseFetches();
    void ResumeFetches();
private:
    // A federated joiner's avatar lives on their home master, so a cache miss means an HTTP call.
    // That must not happen on the event-loop thread: every HTTP server in this process shares one
    // event base pumped by one thread, so blocking it stops a master hosted in this same program
    // from ever answering, and the fetch stalls until it times out. The pull runs on a worker and
    // the reply is deferred.
    struct PendingAvatar {
        evhttp_request*    Request    {nullptr};
        evhttp_connection* Connection {nullptr};
        std::atomic<bool>  ClientConnected {true};
        int64_t            UserId {0};
    };

    static void OnClientDisconnect(evhttp_connection *conn, void *arg);
    void FinishFederatedFetch(std::shared_ptr<PendingAvatar> pending);

    void ServeLocal(evhttp_request *req);
    // The non-federated part of ServeLocal, used once a federated lookup has been resolved.
    void ServeFromLocalSources(evhttp_request *req);

    ServerEmulator* mEmu;
    std::set<std::shared_ptr<PendingAvatar>> mPending;
    bool mActive {true};
};
}
