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
// File: TrampolineIntegrityBypass.cpp
// Started by: Hattozo
// Started on: 8/9/2026
// Description:
#include "Patches.h"
#include <windows.h>

#if defined(_M_X64)
void NoobHook::Patches::InstallTrampolineIntegrityBypass() {
	auto patternA = hook::pattern(
		"48 8B C4 48 89 58 ? 48 89 68 ? 48 89 70 ? 48 89 78 ? 41 56 48 83 EC 20 33 DB");
	if (!patternA.count_hint(1).empty()) {
		Out("TrampolineBypass", "Type A match -> ret 1");
		uintptr_t* address = patternA.get(0).get<uintptr_t>(0);
		NoobHook::WriteMemory<uint8_t>(reinterpret_cast<uintptr_t>(address),
			{ 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3 });
	} else {
		Out("TrampolineBypass", "Type A signature not found (wrong build?)");
	}

	auto patternB = hook::pattern(
		"48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC 20 33 DB");
	if (!patternB.count_hint(1).empty()) {
		Out("TrampolineBypass", "Type B match -> ret 1");
		uintptr_t* address = patternB.get(0).get<uintptr_t>(0);
		NoobHook::WriteMemory<uint8_t>(reinterpret_cast<uintptr_t>(address),
			{ 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3 });
	} else {
		Out("TrampolineBypass", "Type B signature not found (wrong build?)");
	}
}
#endif
