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
// File: Hook.h
// Started by: Hattozo
// Started on: 3/16/2025
// Description:
#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "ntdll.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <format>
#include <optional>

namespace NoobHook {
extern FILE* gFile;
extern uint16_t gEmuHttpsPort;
extern uint16_t gEmuHttpPort;
void Out(const char* category, const char* format, ...);

template<typename T>
std::vector<T> ReadMemory(uintptr_t address, size_t count) {
    if (address < 0x10000 || count == 0) return {};
    std::vector<T> buffer(count);
    const SIZE_T bytes = count * sizeof(T);
    SIZE_T read = 0;
    if (!ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(address), buffer.data(), bytes, &read) || read != bytes)
        return {};
    return buffer;
}

template<typename T>
void WriteMemory(uintptr_t address, const std::vector<T>& data) {
    const size_t bytes = data.size() * sizeof(T);
    DWORD old_protection;
    if (!VirtualProtect(reinterpret_cast<LPVOID>(address), bytes, PAGE_EXECUTE_READWRITE, &old_protection))
        return;
    memcpy(reinterpret_cast<void*>(address), data.data(), bytes);
    VirtualProtect(reinterpret_cast<LPVOID>(address), bytes,
        old_protection, &old_protection);
}

template<typename T>
T ReadPrimitive(uintptr_t address) {
    T value {};
    MEMORY_BASIC_INFORMATION bi {};

    if (VirtualQueryEx(GetCurrentProcess(), reinterpret_cast<LPCVOID>(address), &bi, sizeof(bi)) == 0)
        return T {};

    constexpr DWORD readable = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
    if (bi.State != MEM_COMMIT || !(bi.Protect & readable) || (bi.Protect & PAGE_GUARD))
        return T {};

    SIZE_T read = 0;
    NTSTATUS st = NtReadVirtualMemory(GetCurrentProcess(), reinterpret_cast<PVOID>(address), &value, sizeof(value), &read);
    if (!NT_SUCCESS(st) || read != sizeof(value))
        return T {};

    return value;
}

template <typename T>
bool WritePrimitive(std::uintptr_t address, const T& value) {
    SIZE_T bytesWritten;
    DWORD oldProtection;

    if (!VirtualProtectEx(GetCurrentProcess(), reinterpret_cast<LPVOID>(address), sizeof(value), PAGE_READWRITE, &oldProtection)) {
        return false;
    }

    if (NtWriteVirtualMemory(GetCurrentProcess(), reinterpret_cast<PVOID>(address), (PVOID)&value, sizeof(value), &bytesWritten) || bytesWritten != sizeof(value)) {
        return false;
    }

    DWORD d;
    if (!VirtualProtectEx(GetCurrentProcess(), reinterpret_cast<LPVOID>(address), sizeof(value), oldProtection, &d)) {
        return false;
    }

    return true;
}
}

#ifdef NOOBHOOK_HYPERION
// Hook enable info — read by the injector to write hooks from outside
// (bypasses Windows ACG/DynamicCodePolicy)
struct HookEnableInfo {
    char targetModule[32];
    char targetFunc[64];
    void* detourFunc;
    void** originalFuncStorage;
};

struct HookInfoBlock {
    uint64_t magic;
    HookEnableInfo table[16];
    int count;
};

extern HookInfoBlock g_HookInfo;
#endif
