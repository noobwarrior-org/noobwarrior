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
// File: AuthTicketRedeemHandler.h
// Started by: Hattozo
// Started on: 6/1/2026
// Description:
#pragma once
#include <NoobWarrior/HttpServer/Base/Handler.h>
#include <NoobWarrior/HttpServer/Emulator/AuthUtil.h>

#include <atomic>
#include <memory>
#include <optional>
#include <set>
#include <string>

namespace NoobWarrior {
class ServerEmulator;
class AuthTicketRedeemHandler : public Handler {
public:
    AuthTicketRedeemHandler(ServerEmulator* emu);
    void OnRequest(evhttp_request *req, void *userdata) override;

    // Must run before the server's evhttp is freed, for the same reason AssetHandler::PauseProxy
    // does: a verification still in flight must not reply to a connection that no longer exists.
    void PauseRedeems();
    void ResumeRedeems();

private:
    struct PendingRedeem {
        evhttp_request*    Request    {nullptr};
        evhttp_connection* Connection {nullptr};
        std::atomic<bool>  ClientConnected {true}; // cleared if the client hangs up mid-check

        std::string Ticket;
        bool        AllowGuests {false};
    };

    static void OnClientDisconnect(evhttp_connection *conn, void *arg);

    // Runs back on the event loop once the worker has an answer (or gave up).
    void FinishRedeem(std::shared_ptr<PendingRedeem> pending, std::optional<AuthUtil::SessionUser> resolved);
    // Shared tail: stamps the launch user and writes the reply.
    void ReplyWithUser(evhttp_request *req, const std::optional<AuthUtil::SessionUser> &resolved, bool allowGuests);

    ServerEmulator* mEmu;
    std::set<std::shared_ptr<PendingRedeem>> mPending;
    bool mActive {true};
};
}
