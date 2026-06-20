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
// File: AvatarOverrideHandler.h
// Started by: Hattozo
// Started on: 6/20/2026
// Description: Receives an avatar appearance federated from a joining client. The client only knows
//              its own appearance (stored in its local registry), so when it joins a remote host it
//              POSTs that appearance here, keyed by its user id. The host then serves it from
//              AvatarFetchHandler so its game server builds the joining player's character correctly.
//              Some assistance by Claude Opus 4.8
#pragma once
#include <NoobWarrior/HttpServer/Base/Handler.h>

namespace NoobWarrior {
class ServerEmulator;
class AvatarOverrideHandler : public Handler {
public:
    AvatarOverrideHandler(ServerEmulator* emu);
    void OnRequest(evhttp_request *req, void *userdata) override;
private:
    ServerEmulator* mEmu;
};
}
