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
// File: FixPlayerNullMovssDeref.cpp
// Description: Targeted backstop for the MOVSS [edi=null] site in the function
// starting at RVA 0x835B20 (a sibling of the one patched in
// FixPlayerMissingEdxNullCheck, same code family identified via Studio PDB as
// the DeserializedClusterItem-family processing). That function also loads
//   mov edi, [edx + 0x30]
// in its prologue and later does
//   movss xmm0, [edi]    ; F3 0F 10 07
//   ucomiss xmm0, xmm0    ; standard NaN-check codegen
//   lahf; test ah, 0x44; jp +0x92
// without null-checking edi first. The full 17-byte sequence including the JP
// displacement (0x92) is unique in the binary -- single match -- so we can
// surgically patch this exact site.
//
// Strategy: rewrite the first 5 bytes (MOVSS + first byte of UCOMISS) as a
// JMP rel32 into a small VirtualAlloc'd trampoline that null-checks EDI and,
// on null, jumps to the same destination the in-binary NaN branch would have
// taken -- i.e. treats null as "value is NaN", which the function already has
// logic to handle. Otherwise it executes the original MOVSS+UCOMISS+LAHF+
// TEST+JP and JMPs back to the original fall-through.

#include "Patches.h"
#include <windows.h>
#include <cstring>

void NoobHook::Patches::FixPlayerNullMovssDeref() {
    auto pat = hook::pattern("F3 0F 10 07 0F 2E C0 9F F6 C4 44 0F 8A 92 00 00 00");
    if (pat.count_hint(1).empty()) {
        Out("FixPlayerNullMovssDeref",
            "Pattern not found -- safe to skip (not the affected player build)");
        return;
    }
    uint8_t* crash = pat.get(0).get<uint8_t>(0);

    // JP at crash+11 has rel32 = 0x92; instruction after JP is crash+17.
    // NaN-branch target = (crash + 17) + 0x92 = crash + 0xA9.
    uint8_t* nanTarget   = crash + 0xA9;
    uint8_t* fallThrough = crash + 17;

    uint8_t* tramp = static_cast<uint8_t*>(VirtualAlloc(nullptr, 64,
        MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    if (!tramp) {
        Out("FixPlayerNullMovssDeref", "VirtualAlloc for trampoline failed: %lu", GetLastError());
        return;
    }

    // Trampoline layout (offsets):
    //   00: 85 FF                          test edi, edi
    //   02: 74 16                          jz +0x16  -> NULL_PATH at 26
    //   04: F3 0F 10 07                    movss xmm0, [edi]
    //   08: 0F 2E C0                       ucomiss xmm0, xmm0
    //   11: 9F                             lahf
    //   12: F6 C4 44                       test ah, 0x44
    //   15: 0F 8A <rel32>                  jp NaN_TARGET (absolute fill-in)
    //   21: E9 <rel32>                     jmp FALL_THROUGH
    //   26: E9 <rel32>                     jmp NaN_TARGET  (NULL_PATH)
    //   31: end
    uint8_t t[31] = {
        0x85, 0xFF,
        0x74, 0x16,
        0xF3, 0x0F, 0x10, 0x07,
        0x0F, 0x2E, 0xC0,
        0x9F,
        0xF6, 0xC4, 0x44,
        0x0F, 0x8A, 0, 0, 0, 0,
        0xE9, 0, 0, 0, 0,
        0xE9, 0, 0, 0, 0,
    };
    int32_t jpDisp   = static_cast<int32_t>(nanTarget   - (tramp + 21));
    int32_t fallDisp = static_cast<int32_t>(fallThrough - (tramp + 26));
    int32_t nullDisp = static_cast<int32_t>(nanTarget   - (tramp + 31));
    std::memcpy(t + 17, &jpDisp,   4);
    std::memcpy(t + 22, &fallDisp, 4);
    std::memcpy(t + 27, &nullDisp, 4);
    std::memcpy(tramp, t, sizeof(t));

    // Patch the crash site: 5-byte JMP rel32 to the trampoline.
    int32_t patchDisp = static_cast<int32_t>(tramp - (crash + 5));
    uint8_t patch[5] = { 0xE9, 0, 0, 0, 0 };
    std::memcpy(patch + 1, &patchDisp, 4);
    NoobHook::WriteMemory(reinterpret_cast<uintptr_t>(crash), patch, sizeof(patch));

    Out("FixPlayerNullMovssDeref",
        "Patched at %p (trampoline %p; nan-target %p, fall-through %p)",
        crash, tramp, nanTarget, fallThrough);
}
