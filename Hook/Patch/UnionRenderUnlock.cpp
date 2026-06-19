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
// File: UnionRenderUnlock.cpp
// Started by: Hattozo
// Started on: 6/19/2026
// Description: Un-gate the 0.574 player's NEW (de-interleaved / SoA) CSG vertex render path so 2026-format
//   unions actually DISPLAY instead of rendering blank. The render builder (0x8364d0 / 0x836890) chooses
//   between a LEGACY interleaved vertex array at geom+0x10 (84B stride; NULL for 2026 unions) and the NEW
//   de-interleaved float vector at geom+0x1c (12B stride), via:
//        cmp [geom+4],5 ; jl legacy        ; version selector
//        cmp byte [0x3d0bb20],0 ; je legacy ; a FastFlag (operand is the PREFERRED-base VA -> RVA 0x390bb20)
//   The byte at RVA 0x390bb20 is a FastFlag whose compiled default is 0 in this client (Studio's build has
//   it on), so the new path is never taken -> legacy null read -> ClusterNullGuard zeros it -> empty union.
//   We flip the FastFlag to 1 so geom+4>=5 geometry takes the new path and consumes the real 2026 SoA
//   vertices; legacy geometry (geom+4<5) is unaffected by the branch. The FFlag registration system defaults
//   it to 0 during startup, so a small thread re-asserts 1 (it has no other writer in this client).
//   x86-only, signature-gated to the 0.574.0.38814 build. EXPERIMENTAL: confirms whether geom+0x70 is
//   populated for these unions -- if they appear, the theory holds; if other geometry blanks, revert.
#include "Patches.h"
#include <windows.h>
#include <cstdint>
#include <cstring>

#if defined(_M_IX86)
namespace NoobHook {
namespace {
const uintptr_t kFFlagRva = 0x390bb20;   // new-vertex-format FastFlag byte (RVA; operand 0x3d0bb20 is the preferred-base VA)
// 0.574 build fingerprint: the subcount lea we pinned for the CSG heap fix (CsgHeapGuard).
const uintptr_t kFpRva    = 0x9109e7;
const uint8_t   kFpSig[]  = { 0x8d, 0x34, 0xc5, 0x08, 0x00, 0x00, 0x00 };

DWORD WINAPI ReassertThread(LPVOID base_) {
    volatile uint8_t* flag = reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(base_) + kFFlagRva);
    bool announced = false;
    // FFlag registration runs during player startup and defaults this to 0; keep it 1 for the session.
    for (;;) {
        if (*flag != 1) {
            *flag = 1;
            if (!announced) { Out("UnionRender", "flipped new-vertex-format FFlag @ rva 0x%zx -> 1", (size_t)kFFlagRva); announced = true; }
        }
        Sleep(250);
    }
}

bool BuildIs0574(uintptr_t base) {
    auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    auto* nt  = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || nt->Signature != IMAGE_NT_SIGNATURE)
        return false;
    if (kFpRva + sizeof(kFpSig) > nt->OptionalHeader.SizeOfImage)
        return false;
    return std::memcmp(reinterpret_cast<const void*>(base + kFpRva), kFpSig, sizeof(kFpSig)) == 0;
}
} // anonymous namespace

void Patches::InstallUnionRenderUnlock() {
    uintptr_t base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    if (!BuildIs0574(base)) {
        Out("UnionRender", "non-0.574 build -> render unlock DISARMED");
        return;
    }
    DWORD oldProt = 0;                       // FFlag bytes live in .data (RW); ensure writable to be safe
    VirtualProtect(reinterpret_cast<void*>(base + kFFlagRva), 1, PAGE_READWRITE, &oldProt);
    HANDLE h = CreateThread(nullptr, 0, ReassertThread, reinterpret_cast<LPVOID>(base), 0, nullptr);
    if (h) CloseHandle(h);
    Out("UnionRender", "new-vertex-format render unlock armed (FFlag @ rva 0x%zx)", (size_t)kFFlagRva);
}
} // namespace NoobHook
#endif
