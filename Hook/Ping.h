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
//              /v1/process-ping endpoint so the host process can track which
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
    int Port {0};          // 0 = absent
    int64_t PlaceId {0};   // 0 = absent
};

/* Detects side from the current process executable name. */
ProcessSide DetectSide();

/* Best-effort scan of gameserver.json next to the running exe for server-side params.
 * Sets info.Port / info.PlaceId on success. No-op if file missing or malformed. */
void ReadGameServerJson(ProcessInfo *info);

/* Build a complete ProcessInfo for the current process (PID, side, version, server params if any). */
ProcessInfo CollectProcessInfo();

/* POSTs a JSON Hello to http://127.0.0.1:8080/v1/process-ping. Returns true on send. */
bool SendHello(const ProcessInfo &info);

/* POSTs a JSON Goodbye. Designed to be safe to call from DllMain DLL_PROCESS_DETACH. */
bool SendGoodbye(int pid);

/* POSTs a JSON Heartbeat carrying the full ProcessInfo. Used by the lifecycle watchdog so
 * the emulator can reap instances whose process died without sending Goodbye. Carrying the
 * full info means the emulator can re-register us from scratch if it was restarted. */
bool SendHeartbeat(const ProcessInfo &info);
}
