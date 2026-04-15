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
// File: RemoveSignatureCheck.cpp
// Started by: Hattozo
// Started on: 3/24/2026
// Description:
#include "Patches.h"
#include <windows.h>

void NoobHook::Patches::RemoveSignatureCheck() {
    auto pattern = hook::pattern("CC CC CC CC CC CC CC CC CC CC CC CC CC CC 55 8B EC 6A FF 68 50 D6");
    if (!pattern.count_hint(1).empty()) {
		printf("Found rbxsig pattern\n");
        uintptr_t* address = pattern.get(0).get<uintptr_t>(14);
        const uint8_t bytes[] = { 0xC3 };
        NoobHook::WriteMemory(reinterpret_cast<uintptr_t>(address), bytes, sizeof(bytes));
    }
}
