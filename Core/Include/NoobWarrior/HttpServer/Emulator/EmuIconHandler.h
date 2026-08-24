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
// File: EmuIconHandler.h
// Started by: Hattozo
// Started on: 8/24/2026
// Description: Serves this emulator's icon image at one stable URL.
#pragma once
#include <NoobWarrior/HttpServer/Base/Handler.h>

#include <string>
#include <vector>

namespace NoobWarrior {
class ServerEmulator;
class EmuIconHandler : public Handler {
public:
    EmuIconHandler(ServerEmulator* emu);
    void OnRequest(evhttp_request *req, void *userdata) override;
private:
    // Reads the bytes emu.branding.icon points at. False when the key is unset, names something
    // unreadable, or uses a protocol we cannot serve from (http://, rbxassetid://, ...).
    bool LoadConfiguredIcon(std::vector<unsigned char> *out);

    ServerEmulator* mEmu;
};
}