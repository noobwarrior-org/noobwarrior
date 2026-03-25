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
 // File: BypassVersionOutOfDate.cpp
 // Started by: Hattozo
 // Started on: 3/24/2026
 // Description:
#include "Patches.h"
#include <windows.h>

// THIS DOESn'T WORK IT JUST CRASHES UR GAME
void NoobHook::Patches::BypassVersionOutOfDate() {
    auto pattern = hook::pattern("0F 87 83 00 00 00 0F B6 80 E0 10 72 01 FF 24 85 D0 10 72 01");
    if (!pattern.count_hint(1).empty()) {
        MessageBoxA(0, "Found version check pattern!", "noobHook", 0);
        uintptr_t* address = pattern.get(0).get<uintptr_t>(0);
        const uint8_t bytes[] = { 0xE9, 0x82, 0x00, 0x00, 0x00, 0x90 };
        NoobHook::WriteMemory(reinterpret_cast<uintptr_t>(address), bytes, sizeof(bytes));
    }
}
