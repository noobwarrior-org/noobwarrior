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
// File: EmuDbGamesHandler.h
// Started by: Hattozo
// Started on: 8/24/2026
// Description: Lists every playable game across the mounted EmuDb's as JSON, for a running game
// server that wants to enumerate them through HttpService. Loopback only.
#pragma once
#include <NoobWarrior/HttpServer/Base/Handler.h>

namespace NoobWarrior {
class ServerEmulator;
class EmuDbGamesHandler : public Handler {
public:
    EmuDbGamesHandler(ServerEmulator* emu);
    void OnRequest(evhttp_request *req, void *userdata) override;
private:
    ServerEmulator* mEmu;
};
}