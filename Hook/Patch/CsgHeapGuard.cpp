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
// Description: Stopgap (b2) for the 0.574 player's load-phase CSG heap-corruption.
//   The CSG geometry deserializer (RVA 0x910900) overflows a Vector3 vertex buffer when fed a
//   2026-format CSGPHS/TriangleMesh physics stream: it SIZES the temp buffer with multiplier
//   (2*cl + 3) but the writer COMMITS to (cl ? 9 : 3) elements -- they diverge at the live cl=1
//   (alloc 5 vs emit 9), a ~134 KB linear overrun that smashes an adjacent _HEAP_ENTRY. ntdll's
//   lazy validator only trips LATER, when that temp buffer is freed through a small wrapper
//   (RVA 0x22c74d2):  void __cdecl free_wrapper(void* p){ if(p) HeapFree(theHeap,0,p); ... }
//   -> STATUS_HEAP_CORRUPTION, killing the client mid-load.
//
//   This guard hooks that wrapper and runs the free inside an SEH frame: if the block's metadata
//   is already smashed (or the pointer is wild), HeapFree raises a catchable 0xC0000374/0xC0000005
//   which we SWALLOW -- leaking the block instead of crashing, so the level finishes loading. It
//   does NOT undo the overflow (that already happened at the write); it just keeps the client
//   alive. The real cure is the alloc-multiplier fix (b1) at RVA 0x910a35. x86-only; signature-
//   gated to the 0.574.0.38814 build so other builds are left untouched.
#include "Patches.h"
#include <windows.h>
#include <MinHook.h>
#include <atomic>
#include <cstdint>
#include <cstring>

#if defined(_M_IX86)
namespace NoobHook {
namespace {
// 0.574 free-wrapper entry: mov edi,edi; push ebp; mov ebp,esp; cmp [ebp+8],0; je 0x22c750a
const uint8_t   kFreeWrapSig[] = { 0x8b,0xff,0x55,0x8b,0xec,0x83,0x7d,0x08,0x00,0x74,0x2d };
const uintptr_t kFreeWrapRva   = 0x22c74d2;

// b1 (the real cure) -- alloc-multiplier patch site at 0x910a35.
//   lea eax,[eax*2+3]  (alloc multiplier = 2cl+3)  ->  mov eax,edi ; nop*5  (edi = emit multiplier
//   (cl?9:3), set by the cmovne at 0x910a2e and stored to [ebp-0x70] at 0x910a32). This sizes the
//   vertex buffer for exactly what the writer emits, so the cl=1 (2026 CSGPHS) path can't overrun.
//   cl=0 (legacy) is untouched: both formulas yield 3.
const uintptr_t kAllocRva    = 0x910a35;
const uint8_t   kAllocOrig[] = { 0x8d,0x04,0x45,0x03,0x00,0x00,0x00 }; // lea eax,[eax*2+3]
const uint8_t   kAllocFix[]  = { 0x8b,0xc7,0x90,0x90,0x90,0x90,0x90 }; // mov eax,edi ; nop nop nop nop nop

using FreeWrap_t = void (__cdecl*)(void*);
FreeWrap_t pOrigFreeWrapper = nullptr;
std::atomic<unsigned> gSwallowed { 0 };

// Run the real free inside its own SEH frame (separate function so the __try does not force C++
// object unwinding on the detour). A free that raises heap-corruption or an AV means the block was
// already smashed by the overflow -- swallow and leak rather than let it abort the process.
bool GuardedFree(void* p) {
    __try {
        pOrigFreeWrapper(p);
        return true;
    } __except (GetExceptionCode() == 0xC0000374 /*STATUS_HEAP_CORRUPTION*/ ||
                GetExceptionCode() == 0xC0000005 /*ACCESS_VIOLATION*/
                ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return false;
    }
}

void __cdecl MyFreeWrapper(void* p) {
    if (!GuardedFree(p)) {
        unsigned n = ++gSwallowed;
        if (n <= 8 || (n % 64) == 0)
            Out("CsgGuard", "swallowed corrupt free of %p -> leaked, no crash (total=%u)", p, n);
    }
}

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

// Call AFTER MH_Initialize() and BEFORE MH_EnableHook(MH_ALL_HOOKS) in Thread().
void Patches::InstallCsgHeapGuard() {
    uintptr_t base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));

    // b1 (PRIMARY, the cure): size the CSG vertex alloc with the emit multiplier so the 2026 path
    // can't overrun. Prevents the corruption at its source -- nothing downstream sees a bad heap.
    ApplyBytePatch(base, kAllocRva, kAllocOrig, kAllocFix, sizeof(kAllocOrig), "CSG alloc-mult fix (b1)");

    // b2 (BACKSTOP only): swallow a corrupt free at the one wrapper we pinned. NOTE this is whack-a-mole
    // -- the corruption surfaces at whatever free touches the smashed region first (seen at both
    // 0x22c74ee and 0x22c7b6d), so b2 alone is unreliable; with b1 in place it should never fire.
    auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    auto* nt  = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    if (dos->e_magic == IMAGE_DOS_SIGNATURE && nt->Signature == IMAGE_NT_SIGNATURE &&
        kFreeWrapRva + sizeof(kFreeWrapSig) <= nt->OptionalHeader.SizeOfImage &&
        std::memcmp(reinterpret_cast<const void*>(base + kFreeWrapRva), kFreeWrapSig, sizeof(kFreeWrapSig)) == 0) {
        MH_STATUS st = MH_CreateHook(reinterpret_cast<LPVOID>(base + kFreeWrapRva),
                                     reinterpret_cast<LPVOID>(&MyFreeWrapper),
                                     reinterpret_cast<LPVOID*>(&pOrigFreeWrapper));
        Out("CsgGuard", "CSG free-wrapper backstop %s (rva 0x%zx)", st == MH_OK ? "ARMED" : "FAILED", (size_t)kFreeWrapRva);
    } else {
        Out("CsgGuard", "free-wrapper signature mismatch @ rva 0x%zx -> backstop disarmed", (size_t)kFreeWrapRva);
    }
}
} // namespace NoobHook
#endif
