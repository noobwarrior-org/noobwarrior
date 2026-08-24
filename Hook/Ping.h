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
// File: Ping.h
// Started by: Hattozo
// Started on: 5/13/2026
// Description: Lifecycle pings (Hello/Goodbye) sent to the server emulator's
//              /emu/v1/process-ping endpoint so the host process can track which
//              Roblox instances are currently alive.
#pragma once
#include <cstdint>

namespace NoobHook {
enum class ProcessSide {
    Unknown,
    Client,
    Server,
    Studio
};

struct ProcessInfo {
    int Pid {0};
    ProcessSide Side {ProcessSide::Unknown};
    const char* Version {""};
    const char* Hash {""};  // engine folder name ("version-<hash>"); the client-version upload id
    int Port {0};          // 0 = absent
    int64_t PlaceId {0};   // 0 = absent
};

ProcessSide DetectSide();
void ReadGameServerJson(ProcessInfo *info);
void ReadServerEnvFallback(ProcessInfo *info);
ProcessInfo CollectProcessInfo();
bool SendHello(const ProcessInfo &info);
bool SendGoodbye(int pid);
bool SendHeartbeat(const ProcessInfo &info);
}
