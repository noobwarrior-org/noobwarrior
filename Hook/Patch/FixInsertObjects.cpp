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
 // File: FixInsertObjects.cpp
 // Started by: Hattozo
 // Started on: 5/23/2026
 // Description: Thanks to VisualPlugin for this patch https://github.com/Windows81/Roblox-Freedom-Distribution-Guides/tree/main/InsertObjects2021E
#include "Patches.h"
#include <windows.h>

void NoobHook::Patches::FixInsertObjects() {
    auto pattern = hook::pattern("5F 5E 5B C3 48 8D 15 A4 81 06 02");
    if (!pattern.count_hint(1).empty()) {
        Out("FixInsertObjects", "Found pattern 1 for insert objects functionality");
        uintptr_t* address = pattern.get(0).get<uintptr_t>(2);
        const uint8_t bytes[] = { 0xEB, 0x51 };
        NoobHook::WriteMemory(reinterpret_cast<uintptr_t>(address), bytes, sizeof(bytes));
    }

    auto pattern2 = hook::pattern("E8 88 A2 9D 01 CC CC CC CC CC CC");
    if (!pattern2.count_hint(1).empty()) {
        Out("FixInsertObjects", "Found pattern 2 for insert objects functionality");
        uintptr_t* address = pattern2.get(0).get<uintptr_t>(5);
        const uint8_t bytes[] = { 0x5B, 0xE9, 0xD4, 0x6F, 0xF6, 0xFF };
        NoobHook::WriteMemory(reinterpret_cast<uintptr_t>(address), bytes, sizeof(bytes));
    }
}
