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
// File: FixPlayerMissingEdxNullCheck.cpp
// Description: Smart guard for the 0.573 RobloxPlayerBeta helper at RVA
// 0x835AC0 that's invoked during DeserializedClusterItem-family processing
// (identified via Studio 0.548 PDB -- this is one of the small per-item
// "fill output buffer from input chunks" helpers; its caller is the inlined
// iteration inside Replicator::process*). Signature is __fastcall:
//   ecx = `this` (output buffer; output cursor starts at this+0xD)
//   edx = `input` (read cursor starts at edx+0x20, stride 0x54)
//   arg1 [ebp+8]  = `count` (the loop is `do { ... } while (--count)`)
//   arg2 [ebp+0xC] = `arg2` (a single byte, written into each output record)
//
// Function layout (RVA 0x835AC0, 0x60 bytes total):
//   0x00..0x15: prologue (push ebp, edi=count, jz +0x4D to short exit at
//               offset 0x58 if count==0, push ebx, esi, eax=this+0xD, esi=edx+0x20)
//   0x16..0x1F: 10-byte alignment NOP slot  <-- WHERE WE PATCH
//   0x20..0x55: loop body (each iter writes 0x18 bytes to [eax-0x25..eax-0x0E],
//               advances eax by 0x18, advances esi by 0x54, decrements edi)
//   0x56..0x5A: full epilogue (pop esi, ebx, edi, ebp; ret)
//
// The bug: the compiler emitted `do { read input; ... ; --count; } while (count);`
// so when the caller passes `edx == null` with `count > 0`, the loop body
// executes once -- the input read at [esi-0x20] = [0] AVs.
//
// PRIOR (now-replaced) patch just inserted `test edx,edx; jz exit` at the
// NOP slot. That stopped the crash but made the function early-return without
// populating the output -- callers expected `count * 0x18` bytes of output
// initialized, and downstream code AV'd calling a null function pointer
// inside that uninitialized buffer.
//
// SMART version: on null edx we substitute a small VirtualAlloc'd trampoline
// that runs the SAME loop COUNT times, but writes the all-zero-input
// equivalent of each iteration's output:
//   [eax-0x25..eax-0x1E] = 8 bytes of 0   (would be [esi-0x20] qword)
//   [eax-0x1D..eax-0x1A] = 4 bytes of 0   (would be [esi-0x6C] dword)
//   [eax-0x19]            = bl            (the arg2 byte -- arg2 is value-typed,
//                                          unchanged on null input)
//   [eax-0x18..eax-0x17] = 0              (literal zero word in original)
//   [eax-0x16]            = 0             (would be [esi-0x59] byte)
//   [eax-0x15..eax-0x12] = 4 bytes of 0   (would be [esi-0x54] dword)
//   [eax-0x11..eax-0x0E] = 4 bytes of 0   (would be [esi-0x50] dword)
//   ; advance eax by 0x18
// Then `dec edi; jnz top` until count == 0, then jmp to the full epilogue at
// offset 0x56 (4-pop). The output buffer is fully initialized; EAX is left at
// `this + 0xD + 0x18*count` matching what the real loop would have returned.

#include "Patches.h"
#include <windows.h>
#include <cstring>

void NoobHook::Patches::FixPlayerMissingEdxNullCheck() {
    auto pat = hook::pattern(
        "55 8B EC 57 8B 7D 08 85 FF 74 4D 53 8B 5D 0C 8D "
        "41 0D 56 8D 72 20 66 66 0F 1F 84 00 00 00 00 00"
    );
    if (pat.count_hint(1).empty()) {
        Out("FixPlayerMissingEdxNullCheck",
            "Pattern not found -- safe to skip (not the affected player build)");
        return;
    }
    uint8_t* funcStart = pat.get(0).get<uint8_t>(0);
    uint8_t* nopSlot   = funcStart + 0x16;
    uint8_t* funcExit  = funcStart + 0x56; // 4-pop epilogue (pop esi/ebx/edi/ebp; ret)

    uint8_t* tramp = static_cast<uint8_t*>(VirtualAlloc(nullptr, 64,
        MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    if (!tramp) {
        Out("FixPlayerMissingEdxNullCheck", "VirtualAlloc failed: %lu", GetLastError());
        return;
    }

    // Trampoline -- safe loop that writes the zero-input-equivalent of each
    // original iter's output, runs `edi` times, then jumps to the 4-pop epilogue.
    // Offsets are relative to the start of the trampoline.
    uint8_t t[64] = {
        // loop_top (off 0):
        0x33, 0xC9,                                   //  0 (2)  xor ecx, ecx
        0x8D, 0x40, 0x18,                             //  2 (3)  lea eax, [eax+0x18]
        0x89, 0x48, 0xDB,                             //  5 (3)  mov [eax-0x25], ecx
        0x89, 0x48, 0xDF,                             //  8 (3)  mov [eax-0x21], ecx
        0x89, 0x48, 0xE3,                             // 11 (3)  mov [eax-0x1D], ecx
        0x88, 0x58, 0xE7,                             // 14 (3)  mov [eax-0x19], bl  <-- arg2
        0x66, 0xC7, 0x40, 0xE8, 0x00, 0x00,           // 17 (6)  mov word ptr [eax-0x18], 0
        0xC6, 0x40, 0xEA, 0x00,                       // 23 (4)  mov byte ptr [eax-0x16], 0
        0x89, 0x48, 0xEB,                             // 27 (3)  mov [eax-0x15], ecx
        0x89, 0x48, 0xEF,                             // 30 (3)  mov [eax-0x11], ecx
        0x83, 0xEF, 0x01,                             // 33 (3)  sub edi, 1
        0x75, 0xDA,                                   // 36 (2)  jnz loop_top  (rel8 = -38)
        // off 38: jmp funcExit (rel32)
        0xE9, 0, 0, 0, 0,                             // 38 (5)
        // (rest zero -- unused)
    };
    int32_t exitDisp = static_cast<int32_t>(funcExit - (tramp + 43));
    std::memcpy(t + 39, &exitDisp, 4);
    std::memcpy(tramp, t, 43);

    // Patch the NOP slot:
    //   off 0x16: 85 D2          test edx, edx
    //   off 0x18: 75 06          jnz +6   (skips the 5-byte E9 + 1-byte nop -> lands at offset 0x20 = loop body)
    //   off 0x1A: E9 disp32      jmp <trampoline>
    //   off 0x1F: 90             nop      (padding so the slot is exactly 10 bytes)
    uint8_t patch[10] = {
        0x85, 0xD2,
        0x75, 0x06,
        0xE9, 0, 0, 0, 0,
        0x90,
    };
    int32_t patchDisp = static_cast<int32_t>(tramp - (nopSlot + 9));
    std::memcpy(patch + 5, &patchDisp, 4);
    NoobHook::WriteMemory(reinterpret_cast<uintptr_t>(nopSlot), patch, sizeof(patch));

    Out("FixPlayerMissingEdxNullCheck",
        "Patched at %p (trampoline %p; exit %p)", funcStart, tramp, funcExit);
}
