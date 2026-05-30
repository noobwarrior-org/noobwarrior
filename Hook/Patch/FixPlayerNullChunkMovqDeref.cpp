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
// File: FixPlayerNullChunkMovqDeref.cpp
// Description: Targeted patch for a sibling helper in the same
// DeserializedClusterItem family as FixPlayerMissingEdxNullCheck (identified
// via Studio 0.548 PDB). The helper at RVA 0x83574A has the prologue
//   56              push esi
//   8D 71 0D        lea esi, [ecx+0xD]
//   66 90           nop
//   F3 0F 7E 42 F0  movq xmm0, [edx-0x10]   <-- AVs when caller passed
//                                               edx as a small sentinel (e.g. 0x10)
// The function reads many fields off edx (offsets -0x10, -8, +0x14, +0x28, ...)
// and writes them through esi. There's no null check before the first MOVQ at
// RVA 0x835750. Pattern verified unique (16-byte signature).
//
// Strategy: overwrite the 5-byte MOVQ at the crash site with a JMP rel32 into
// a small VirtualAlloc'd trampoline. The trampoline:
//   - cmp edx, 0x10000   (low-VA sentinel detection)
//   - if edx is small/invalid -> pop esi (balance prologue) ; ret
//   - else execute the original MOVQ and JMP back to the next instruction.

#include "Patches.h"
#include <windows.h>
#include <cstring>

void NoobHook::Patches::FixPlayerNullChunkMovqDeref() {
    // Anchor on the FUNCTION PROLOGUE (push esi; lea esi, [ecx+0xD]; nop) +
    // the first byte of the MOVQ. The bare MOVQ sequence is NOT unique --
    // there's a sibling helper that uses the same MOVQ shape but a different
    // prologue. Without the anchor we'd mis-patch the wrong site.
    auto pat = hook::pattern("56 8D 71 0D 66 90 F3 0F 7E 42 F0 66 0F D6 46 F3");
    if (pat.count_hint(1).empty()) {
        Out("FixPlayerNullChunkMovqDeref",
            "Pattern not found -- safe to skip (not the affected player build)");
        return;
    }
    // The MOVQ starts 6 bytes into the matched range (after push esi; lea esi; nop).
    uint8_t* crash = pat.get(0).get<uint8_t>(0) + 6;

    uint8_t* tramp = static_cast<uint8_t*>(VirtualAlloc(nullptr, 64,
        MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    if (!tramp) {
        Out("FixPlayerNullChunkMovqDeref", "VirtualAlloc failed: %lu", GetLastError());
        return;
    }

    // The enclosing function actually starts at RVA 0x835720 with a frame
    // prologue (push ebp; mov ebp, esp; push edi; ... push esi at offset 0x2A).
    // By the time our MOVQ runs, THREE registers have been saved (ebp, edi,
    // esi). The function's full 4-pop epilogue is at RVA 0x8358A9:
    //   5E 5F 5D C3   pop esi ; pop edi ; pop ebp ; ret
    // The smart early-exit jumps STRAIGHT THERE so cleanup matches the real
    // function's exit -- previously I tried "pop esi ; ret" inside the
    // trampoline which left edi+ebp on the stack and made ret consume edi
    // (which was 0), producing the spurious "EIP=0" call-to-null crash.
    //
    // Trampoline layout:
    //   00..05: 81 FA 00 00 01 00     cmp edx, 0x10000
    //   06..0B: 0F 82 0D 00 00 00     jb +0x0D  -> early_exit @ offset 0x19
    //   0C..10: F3 0F 7E 42 F0        movq xmm0, [edx-0x10]   (original)
    //   11..15: E9 <rel32>             jmp <crash + 5>
    //   16..18: 90 90 90               nop x3 (padding)
    //   19..1D: E9 <rel32>             jmp <funcExit = crash - 0x150 + ...>
    uint8_t* funcExit = crash - 0x750 + 0x58A9; // RVA: 0x8358A9 = 0x835750 + 0x159; crash is +0x750 ... compute simpler:
    // crash is the address of the MOVQ at RVA 0x835750. The exit is at RVA 0x8358A9.
    funcExit = crash + (0x8358A9 - 0x835750);

    uint8_t t[64] = {
        0x81, 0xFA, 0x00, 0x00, 0x01, 0x00,   // 00 cmp edx, 0x10000
        0x0F, 0x82, 0x0D, 0x00, 0x00, 0x00,   // 06 jb +0x0D -> early_exit @ offset 0x19
        0xF3, 0x0F, 0x7E, 0x42, 0xF0,         // 0C movq xmm0, [edx-0x10]
        0xE9, 0, 0, 0, 0,                     // 11 jmp <crash + 5>     (back-to-normal)
        0x90, 0x90, 0x90,                     // 16 nop x3
        0xE9, 0, 0, 0, 0,                     // 19 jmp <funcExit>      (4-pop epilogue)
    };
    int32_t backDisp = static_cast<int32_t>((crash + 5) - (tramp + 22));
    int32_t exitDisp = static_cast<int32_t>(funcExit - (tramp + 30));
    std::memcpy(t + 18, &backDisp, 4);
    std::memcpy(t + 26, &exitDisp, 4);
    std::memcpy(tramp, t, 30);

    // Overwrite the crash site (5 bytes of original MOVQ) with JMP rel32 to trampoline.
    int32_t patchDisp = static_cast<int32_t>(tramp - (crash + 5));
    uint8_t patch[5] = { 0xE9, 0, 0, 0, 0 };
    std::memcpy(patch + 1, &patchDisp, 4);
    NoobHook::WriteMemory(reinterpret_cast<uintptr_t>(crash), patch, sizeof(patch));

    Out("FixPlayerNullChunkMovqDeref",
        "Patched at %p (trampoline %p)", crash, tramp);
}
