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
// File: RemoveTrustCheck.cpp
// Started by: Hattozo
// Started on: 3/22/2026
// Description: Thanks to Worships for making this guide
// https://github.com/Windows81/Roblox-Freedom-Distribution-Guides/tree/main/Worships2021EGuide
#include "Patches.h"
#include <windows.h>

// This patch shouldn't be used unless if you know what you're doing
void NoobHook::Patches::RemoveTrustCheck() {
    auto pattern = hook::pattern("0F 85 AE 00 00 00 83 7D D0 10");
    if (!pattern.count_hint(1).empty()) {
        //MessageBoxA(0, "FOUND TRUST CHECK", "noobHook", 0);
        printf("Found pattern for trust check 1\n");
        uintptr_t* address = pattern.get(0).get<uintptr_t>(0);
        const uint8_t bytes[] = { 0xE9, 0xAF, 0x00, 0x00, 0x00, 0x90 };
        NoobHook::WriteMemory(reinterpret_cast<uintptr_t>(address), bytes, sizeof(bytes));
    }

    auto anotherPattern = hook::pattern("75 58 83 7E 14 10 72 02");
    if (!anotherPattern.count_hint(1).empty()) {
        //MessageBoxA(0, "FOUND TRUST CHECK 2", "noobHook", 0);
        printf("Found pattern for trust check 2\n");
        uintptr_t* address = anotherPattern.get(0).get<uintptr_t>(0);
        const uint8_t bytes[] = { 0xEB };
        NoobHook::WriteMemory(reinterpret_cast<uintptr_t>(address), bytes, sizeof(bytes));
    }

    // for getting rid of the "Non-trusted BaseURL used. HttpRbxApiService is only for Roblox API calls." error in RCCService
    auto rccServicePattern = hook::pattern("75 4F 6A 49 68 70 40 90 01");
    if (!rccServicePattern.count_hint(1).empty()) {
        //MessageBoxA(0, "FOUND RCCSERVICE TRUST CHECK", "noobHook", 0);
        printf("Found pattern for trust check 3\n");
        uintptr_t* address = rccServicePattern.get(0).get<uintptr_t>(0);
        const uint8_t bytes[] = { 0xEB };
        NoobHook::WriteMemory(reinterpret_cast<uintptr_t>(address), bytes, sizeof(bytes));
    }
}
