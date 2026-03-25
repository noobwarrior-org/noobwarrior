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
// File: RemoveTLSVerification.cpp
// Started by: Hattozo
// Started on: 3/22/2026
// Description: Thanks to VisualPlugin for making this guide
// https://github.com/Windows81/Roblox-Freedom-Distribution-Guides/blob/main/PatchTLSVerification/README.md
#include "Patches.h"
#include <windows.h>

void NoobHook::Patches::RemoveTLSVerification() {
    auto disableVerifyPeer = hook::pattern("6A 01 6A 40 FF B7 48 01 00 00");
    if (!disableVerifyPeer.count_hint(1).empty()) {
        //MessageBoxA(0, "Found CURL SSL verify peer pattern!", "noobHook", 0);
        uintptr_t* address = disableVerifyPeer.get(0).get<uintptr_t>(1);
        const uint8_t bytes[] = { 0x00 };
        NoobHook::WriteMemory(reinterpret_cast<uintptr_t>(address), bytes, sizeof(bytes));
    }

    // This pattern does not exist on RCCService for some reason
    auto disableVerifyHost = hook::pattern("6A 02 6A 51 FF B7 48 01 00 00");
    if (!disableVerifyHost.count_hint(1).empty()) {
        //MessageBoxA(0, "Found CURL SSL verify host pattern!", "noobHook", 0);
        uintptr_t* address = disableVerifyHost.get(0).get<uintptr_t>(1);
        const uint8_t bytes[] = { 0x00 };
        NoobHook::WriteMemory(reinterpret_cast<uintptr_t>(address), bytes, sizeof(bytes));
    }
}
