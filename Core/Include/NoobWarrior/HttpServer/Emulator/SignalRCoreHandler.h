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
// File: SignalRCoreHandler.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: SignalR Core WebSocket transport used by recent voice-chat clients.
#pragma once

#include <NoobWarrior/HttpServer/Base/Handler.h>

namespace NoobWarrior {
class Registry;

class SignalRCoreHandler : public Handler {
public:
    explicit SignalRCoreHandler(Registry *registry);
    void OnRequest(evhttp_request *req, void *userdata) override;

private:
    Registry *mRegistry;
};
}
