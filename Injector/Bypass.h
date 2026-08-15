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
// File: Bypass.h
// Started by: Hattozo
// Started on: 8/12/2025
// Description: Code responsible for bypassing Hyperion.
// Slopped together by DeepSeek V4 Pro and GPT 5.6 Sol.
#pragma once
#include <windows.h>
#include <cstdint>
#include <vector>
#include <string>

enum class RobloxEra { Unknown, Era463, Era574, Era719 };

struct PatchEntry {
    uintptr_t rva;
    std::vector<uint8_t> bytes;
};

inline const wchar_t* EraName(RobloxEra era) {
    switch (era) {
    case RobloxEra::Era463: return L"0.463";
    case RobloxEra::Era574: return L"0.574";
    case RobloxEra::Era719: return L"0.719";
    default:                return L"unknown";
    }
}

RobloxEra DetectEra(const wchar_t* exePath);

inline bool NeedsBypass(RobloxEra era) {
    return era == RobloxEra::Era719;
}

inline bool UsesEarlyWspRedirect(RobloxEra era) {
    return era == RobloxEra::Era719;
}

uintptr_t GetRemoteModuleBase(HANDLE hProcess, const wchar_t* moduleName);
uintptr_t GetRemoteModuleBase(DWORD pid, const wchar_t* moduleName);
uintptr_t FindModuleWithSection(DWORD pid, const char* sectionName);
bool HasSectionOnDisk(const wchar_t* exePath, const char* sectionName);
void SuspendProcess(DWORD pid);
void ResumeProcess(DWORD pid);

// Restore Hyperion's inline hooks on ntdll exports.
void UnhookNtFunctions(HANDLE hProcess, DWORD pid, uintptr_t byfronModuleBase = 0);

bool BypassHyperion(HANDLE hProcess, DWORD pid, const wchar_t* exePath);

// Make a module's IAT pages writable before a modern Player enables its late image-page policy.
bool PrepareIatForExternalHooks(HANDLE hProcess, uintptr_t moduleBase);

// Make one resolved-dispatch page writable/private during the same early window.
bool PrepareAddressForExternalHook(HANDLE hProcess, uintptr_t address, const char* label);

// Redirect a modern Player's GetProcAddress import to a loader-free image-backed socket bootstrap while the
// Player IAT is still writable.
bool InstallEarlySocketRedirect(HANDLE hProcess, uintptr_t playerModuleBase,
                                uintptr_t bootstrapBase,
                                const std::vector<uint8_t>& bootstrapData,
                                uint16_t emulatorHttpPort, uint16_t emulatorHttpsPort);

void LogEarlySocketRedirectStats(HANDLE hProcess, uintptr_t bootstrapBase,
                                 const std::vector<uint8_t>& bootstrapData);

// Install the socket redirect through the writable MSWSOCK provider table. The bootstrap
// is mapped before the Player starts; this function is polled as soon as WSPStartup creates the
// provider table and leaves every Roblox/Hyperion/Windows image page unchanged.
size_t InstallEarlyWspRedirect(HANDLE hProcess, uintptr_t bootstrapBase,
                               const std::vector<uint8_t>& bootstrapData,
                               uint16_t emulatorHttpPort, uint16_t emulatorHttpsPort);

enum class LegacyBytecodeFlagState {
    Unsupported,
    WaitingForPlayerInitialization,
    Applied,
    WriteFailed,
};

// Reassert the exact writable FastVariable values used by the 0.719 scheme-0
// bytecode path. This changes no executable or read-only Player image page.
LegacyBytecodeFlagState ReassertLegacyBytecodeFlags(HANDLE hProcess,
                                                    uintptr_t playerBase,
                                                    RobloxEra era);

// Restore IAT pages made temporarily writable before Hyperion enables its process policy.
void RestorePreparedIatProtection(HANDLE hProcess);

// Wait until a manually mapped Hyperion hook publishes its external hook table.
bool WaitForHookInfoReady(HANDLE hProcess, uintptr_t dllBase,
                          const std::vector<uint8_t>& dllData, DWORD timeoutMs);

// Install inline hooks from outside the process (bypasses Windows ACG)
void InstallHooksFromInjector(HANDLE hProcess, uintptr_t dllBase,
                              const std::vector<uint8_t>& dllData,
                              RobloxEra era);
