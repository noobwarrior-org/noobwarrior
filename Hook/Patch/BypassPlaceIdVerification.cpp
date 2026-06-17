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
// File: BypassPlaceIdVerification.cpp
// Started by: Hattozo
// Started on: 3/24/2026
// Description:
#include "Patches.h"
#include <windows.h>

void NoobHook::Patches::BypassPlaceIdVerification() {
    auto pattern = hook::pattern("74 5A 68 38 41 68 02");
    if (!pattern.count_hint(1).empty()) {
        MessageBoxA(0, "Found bypass place id pattern!", "noobHook", 0);
        uintptr_t* address = pattern.get(0).get<uintptr_t>(0);
        NoobHook::WriteMemory<uint8_t>(reinterpret_cast<uintptr_t>(address), { 0xEB });
    }

    auto pattern2 = hook::pattern("0F 84 24 01 00 00 8D 45 E4 50");
    if (!pattern2.count_hint(1).empty()) {
        MessageBoxA(0, "Found bypass place id 2 pattern!", "noobHook", 0);
        uintptr_t* address = pattern2.get(0).get<uintptr_t>(0);
        NoobHook::WriteMemory<uint8_t>(reinterpret_cast<uintptr_t>(address), { 0xE9, 0x23, 0x01, 0x00, 0x00, 0x90 });
    }
}
