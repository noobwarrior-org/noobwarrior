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
// File: LdrLoadShellcode.h
// Started by: Hattozo
// Started on: 8/12/2025
// Description: Shellcode helper for LoadLibrary bypass.
// Writes a small x64 shellcode into the target that calls LdrLoadDll
// with DONT_RESOLVE_DLL_REFERENCES, then calls the DLL's entry point.
// (Slopped by DeepSeek V4 Pro and GPT 5.6 Sol)
//
// TODO: Does anything still reference code from this file anymore?
// Probably not. I just ctrl f'd it and there was nothing.
#pragma once
#include <windows.h>
#include <cstdint>

// Params block written to remote memory before shellcode execution.
struct __declspec(align(16)) LdrLoadParams {
    // Inputs (set by injector)
    HMODULE hNtdll;            // +0x00
    FARPROC pLdrLoadDll;       // +0x08  (resolved in injector's process, same in target)
    UNICODE_STRING dllName;    // +0x10  (UNICODE_STRING pointing at dllPathBuf)
    wchar_t dllPathBuf[520];   // +0x20  (actual path characters + MAX_PATH padding)
    // Outputs
    HMODULE loadedModule;      // +0x430
    NTSTATUS loadStatus;       // +0x438
    FARPROC entryPoint;        // +0x440
};

// x64 shellcode that:
//   RCX = pointer to LdrLoadParams in remote memory
//   Calls LdrLoadDll with DONT_RESOLVE_DLL_REFERENCES to map the DLL
//   Returns the result
inline const uint8_t kLdrLoadShellcode[] = {
    // Save non-volatile regs
    0x48, 0x89, 0x4C, 0x24, 0x08,             // mov [rsp+8], rcx
    0x48, 0x83, 0xEC, 0x38,                     // sub rsp, 0x38
    // Load params
    0x48, 0x8B, 0xD9,                           // mov rbx, rcx (params)
    // Build UNICODE_STRING on stack
    0x48, 0x8D, 0x43, 0x20,                     // lea rax, [rbx+0x20] (dllPathBuf)
    0x48, 0x89, 0x43, 0x10,                     // mov [rbx+0x10], rax (dllName.Buffer = dllPathBuf)
    0x66, 0xC7, 0x43, 0x12, 0x00, 0x00,        // mov word [rbx+0x12], 0 (dllName.Length = 0)
    0x66, 0xC7, 0x43, 0x14, 0x00, 0x08,        // mov word [rbx+0x14], 0x800 (dllName.MaximumLength)
    // strlen of path
    0x48, 0x8D, 0x4B, 0x20,                     // lea rcx, [rbx+0x20]
    0x31, 0xC0,                                  // xor eax, eax
    // strlen_loop:
    0x66, 0x83, 0x3C, 0x41, 0x00,               // cmp word [rcx+rax*2], 0
    0x74, 0x06,                                  // je strlen_done
    0x48, 0xFF, 0xC0,                           // inc rax
    0xEB, 0xF5,                                  // jmp strlen_loop
    // strlen_done:
    0x66, 0x89, 0x43, 0x12,                     // mov [rbx+0x12], ax (dllName.Length = rax*2)
    // Call LdrLoadDll(NULL, 2, &dllName, &loadedModule)
    0x48, 0x8D, 0x8B, 0x30, 0x04, 0x00, 0x00,  // lea rcx, [rbx+0x430] (&loadedModule)  — 4th arg
    0x4C, 0x8D, 0x43, 0x10,                     // lea r8, [rbx+0x10] (&dllName)           — 3rd arg
    0xBA, 0x02, 0x00, 0x00, 0x00,              // mov edx, 2 (DONT_RESOLVE_DLL_REFERENCES) — 2nd arg
    0x31, 0xC9,                                  // xor ecx, ecx (NULL)                     — 1st arg
    0x48, 0x8B, 0x83, 0x00, 0x04, 0x00, 0x00,  // mov rax, [rbx+0x400] (pLdrLoadDll)
    0xFF, 0xD0,                                  // call rax
    // Save status
    0x89, 0x83, 0x38, 0x04, 0x00, 0x00,        // mov [rbx+0x438], eax (loadStatus)
    // Restore and return
    0x48, 0x83, 0xC4, 0x38,                     // add rsp, 0x38
    0xC3,                                        // ret
};
