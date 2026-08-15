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
// File: LocalRccLoader.cpp
// Started by: Hattozo
// Started on: 8/12/2026
// Description: Selects the LocalRcc payload that matches the Studio build.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdint>
#include <cwchar>

namespace {
constexpr DWORD kStudio574TimeDateStamp = 0xc1b36571;
constexpr DWORD kStudio574SizeOfImage = 0x06b42000;
constexpr DWORD kStudio719TimeDateStamp = 0x0c6a7690;
constexpr DWORD kStudio719SizeOfImage = 0x0ccff000;

const wchar_t* SelectPayload() {
    const auto base = reinterpret_cast<const uint8_t*>(GetModuleHandleW(nullptr));
    if (base == nullptr)
        return nullptr;

    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0)
        return nullptr;

    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        return nullptr;

    const DWORD timestamp = nt->FileHeader.TimeDateStamp;
    const DWORD imageSize = nt->OptionalHeader.SizeOfImage;
    if (timestamp == kStudio574TimeDateStamp && imageSize == kStudio574SizeOfImage)
        return L"noobhook_x86-64_localrcc_0574.dll";
    if (timestamp == kStudio719TimeDateStamp && imageSize == kStudio719SizeOfImage)
        return L"noobhook_x86-64_localrcc_0719.dll";
    return nullptr;
}
} // namespace

extern "C" __declspec(dllexport) DWORD WINAPI NoobLocalRccInitialize(LPVOID loaderModule) {
    const wchar_t* payloadName = SelectPayload();
    if (payloadName == nullptr)
        return ERROR_REVISION_MISMATCH;

    wchar_t payloadPath[MAX_PATH] {};
    const DWORD length = GetModuleFileNameW(
        reinterpret_cast<HMODULE>(loaderModule), payloadPath, MAX_PATH);
    if (length == 0 || length >= MAX_PATH)
        return GetLastError() == ERROR_SUCCESS ? ERROR_INSUFFICIENT_BUFFER : GetLastError();

    wchar_t* slash = std::wcsrchr(payloadPath, L'\\');
    if (slash == nullptr)
        return ERROR_BAD_PATHNAME;

    const size_t remaining = MAX_PATH - static_cast<size_t>(slash + 1 - payloadPath);
    if (wcscpy_s(slash + 1, remaining, payloadName) != 0)
        return ERROR_INSUFFICIENT_BUFFER;

    HMODULE payload = LoadLibraryW(payloadPath);
    if (payload == nullptr)
        return GetLastError();

    using Initialize = DWORD (WINAPI*)(LPVOID);
    const auto initialize = reinterpret_cast<Initialize>(
        GetProcAddress(payload, "NoobLocalRccInitialize"));
    if (initialize == nullptr)
        return GetLastError();

    return initialize(payload);
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH)
        DisableThreadLibraryCalls(module);
    return TRUE;
}
