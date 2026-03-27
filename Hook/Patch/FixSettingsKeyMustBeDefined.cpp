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
// File: FixSettingsKeyMustBeDefined.cpp
// Started by: Hattozo
// Started on: 3/22/2026
// Description: Fixes "Settings key must be defined" error for RCCService v0.463
#include "Patches.h"
#include <windows.h>

void NoobHook::Patches::FixSettingsKeyMustBeDefined() {
    auto pattern = hook::pattern("74 2A 0F B6 95 E4 FE FF FF");
    if (!pattern.count_hint(1).empty()) {
        printf("Found pattern for \"settings key must be defined\" message\n");
		//MessageBoxA(0, "Found settings key pattern!", "noobHook", 0);
        uintptr_t* address = pattern.get(0).get<uintptr_t>(0);
        const uint8_t bytes[] = { 0xEB };
		NoobHook::WriteMemory(reinterpret_cast<uintptr_t>(address), bytes, sizeof(bytes));
    }
}
