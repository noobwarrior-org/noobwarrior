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
// File: CsgHeapGuard.cpp
// Started by: Hattozo
// Started on: 6/19/2026
// Description: Fixes the 0.574 player's load-phase CSG heap overflow (STATUS_HEAP_CORRUPTION 0xC0000374).
//   The per-cell CSG/TriangleMesh vertex generator at RVA 0x910900 sizes its temp vertex buffer for
//   `subcount` vertices/cell, computed at 0x9109e7 as `lea esi,[eax*8+8]` = cl_flag*8 + 8 -> 8 when
//   cl_flag==0 (the 2026 path), 16 when cl_flag==1. But the per-cell face TEMPLATE TABLES are 16-wide
//   permutations whose vertex indices reach 15, and the inner writer addresses the buffer as
//   base + (index*count + outer)*M*12. The buffer holds subcount*count slots, so a write is in-bounds
//   iff index < subcount. With subcount=8 and template indices up to 15 it overruns by a full buffer
//   length (~19 KB), smashing the next block's _HEAP_ENTRY; ntdll only trips it later, at the free.
//
//   (M = the alloc multiplier / write stride [ebp-0x50], and `count`, appear in BOTH the alloc size and
//   the write offset, so they CANCEL -- which is why the earlier "fix M" attempt did nothing.)
//
//   FIX: force subcount = 16 unconditionally (patch 0x9109e7 `lea esi,[eax*8+8]` -> `mov esi,0x10; nop;
//   nop`). 16 is exactly what the safe cl_flag==1 path already uses with the same 0..15 tables and the
//   same writers, so the buffer is correctly sized (the slot writers are bounded M*12 memcpys, so the
//   exact fit is safe) -- it prevents the crash and should render correct geometry. Only the cl_flag==0
//   Table-A path is affected; the Table-B (13-wide) path skips this lea and is untouched. x86-only,
//   signature-gated to the 0.574.0.38814 build.
#include "Patches.h"
#include <windows.h>
#include <cstdint>
#include <cstring>

#if defined(_M_IX86)
namespace NoobHook {
namespace {
// subcount lea at 0x9109e7:  lea esi,[eax*8+8]  ->  mov esi,0x10 ; nop ; nop   (force subcount = 16)
const uintptr_t kSubcountRva    = 0x9109e7;
const uint8_t   kSubcountOrig[] = { 0x8d, 0x34, 0xc5, 0x08, 0x00, 0x00, 0x00 }; // lea esi,[eax*8+8]
const uint8_t   kSubcountFix[]  = { 0xbe, 0x10, 0x00, 0x00, 0x00, 0x90, 0x90 }; // mov esi,0x10 ; nop ; nop

// Bounds + signature-checked in-place byte patch. Only writes when the loaded build's bytes match
// `orig` exactly, so a different/stale build is left untouched.
bool ApplyBytePatch(uintptr_t base, uintptr_t rva, const uint8_t* orig, const uint8_t* fix, size_t n,
                    const char* name) {
    auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    auto* nt  = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || nt->Signature != IMAGE_NT_SIGNATURE ||
        rva + n > nt->OptionalHeader.SizeOfImage ||
        std::memcmp(reinterpret_cast<const void*>(base + rva), orig, n) != 0) {
        Out("CsgGuard", "%s signature mismatch @ rva 0x%zx -> skipped (non-0.574 build)", name, (size_t)rva);
        return false;
    }
    DWORD oldProt = 0;
    if (!VirtualProtect(reinterpret_cast<void*>(base + rva), n, PAGE_EXECUTE_READWRITE, &oldProt)) {
        Out("CsgGuard", "%s VirtualProtect failed (err %lu)", name, GetLastError());
        return false;
    }
    std::memcpy(reinterpret_cast<void*>(base + rva), fix, n);
    VirtualProtect(reinterpret_cast<void*>(base + rva), n, oldProt, &oldProt);
    FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(base + rva), n);
    Out("CsgGuard", "%s applied @ rva 0x%zx", name, (size_t)rva);
    return true;
}
} // anonymous namespace

void Patches::InstallCsgHeapGuard() {
    uintptr_t base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    ApplyBytePatch(base, kSubcountRva, kSubcountOrig, kSubcountFix, sizeof(kSubcountOrig),
                   "CSG subcount=16 fix");
}
} // namespace NoobHook
#endif
