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
// File: Bypass.cpp
// Started by: Hattozo
// Started on: 8/12/2025
// Description: Code responsible for bypassing Hyperion.
// Slopped together by DeepSeek V4 Pro and GPT 5.6 Sol.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winternl.h>
#include <psapi.h>
#include <tlhelp32.h>
#include "Bypass.h"
#include "ManualMap.h"
#include "Patches.h"
#include "Syscalls.hpp"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <array>
#include <limits>
#include <vector>
#include <string>

static FILE* gLog = stdout;

struct PreparedIatPage {
    DWORD processId;
    void* address;
    SIZE_T size;
    DWORD originalProtection;
};

static std::vector<PreparedIatPage> gPreparedIatPages;

static void Log(const char* fmt, ...) {
    if (!gLog) return;
    va_list args; va_start(args, fmt);
    fprintf(gLog, "[Bypass] "); vfprintf(gLog, fmt, args);
    fprintf(gLog, "\n"); fflush(gLog);
    va_end(args);
}

static bool IsWritableProtection(DWORD protection) {
    switch (protection & 0xFF) {
    case PAGE_READWRITE:
    case PAGE_WRITECOPY:
    case PAGE_EXECUTE_READWRITE:
    case PAGE_EXECUTE_WRITECOPY:
        return true;
    default:
        return false;
    }
}

static bool IsExecutableProtection(DWORD protection) {
    switch (protection & 0xFF) {
    case PAGE_EXECUTE:
    case PAGE_EXECUTE_READ:
    case PAGE_EXECUTE_READWRITE:
    case PAGE_EXECUTE_WRITECOPY:
        return true;
    default:
        return false;
    }
}

// Roblox rejects protection changes to these image pages by the time external hooks are installed.
// Prepare only the PE IAT pages during the earlier suspended bypass window; the external installer
// can then replace imports without modifying the engine's CA bundle.
bool PrepareIatForExternalHooks(HANDLE hProcess, uintptr_t moduleBase) {
    IMAGE_DOS_HEADER dos = {};
    if (!ReadProcessMemory(hProcess, reinterpret_cast<const void*>(moduleBase),
                           &dos, sizeof(dos), nullptr) ||
        dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew <= 0 || dos.e_lfanew > 0x100000)
        return false;

    IMAGE_NT_HEADERS64 nt = {};
    if (!ReadProcessMemory(hProcess, reinterpret_cast<const void*>(moduleBase + dos.e_lfanew),
                           &nt, sizeof(nt), nullptr) ||
        nt.Signature != IMAGE_NT_SIGNATURE ||
        nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
        nt.OptionalHeader.NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_IAT)
        return false;

    const IMAGE_DATA_DIRECTORY iat = nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT];
    if (!iat.VirtualAddress || !iat.Size ||
        iat.VirtualAddress >= nt.OptionalHeader.SizeOfImage ||
        iat.Size > nt.OptionalHeader.SizeOfImage - iat.VirtualAddress)
        return false;

    SYSTEM_INFO systemInfo = {};
    GetSystemInfo(&systemInfo);
    const uintptr_t pageSize = systemInfo.dwPageSize;
    const uintptr_t pageMask = pageSize - 1;
    const uintptr_t firstPage = (moduleBase + iat.VirtualAddress) & ~pageMask;
    const uintptr_t iatEnd = moduleBase + iat.VirtualAddress + iat.Size;
    const uintptr_t endPage = (iatEnd + pageMask) & ~pageMask;
    const size_t pageCount = static_cast<size_t>((endPage - firstPage) / pageSize);
    const DWORD processId = GetProcessId(hProcess);

    size_t prepared = 0;
    size_t writable = 0;
    size_t privatized = 0;
    for (uintptr_t page = firstPage; page < endPage; page += pageSize) {
        MEMORY_BASIC_INFORMATION memory = {};
        if (VirtualQueryEx(hProcess, reinterpret_cast<const void*>(page),
                           &memory, sizeof(memory)) != sizeof(memory) ||
            memory.State != MEM_COMMIT || (memory.Protect & (PAGE_GUARD | PAGE_NOACCESS))) {
            Log("PrepareIAT: cannot query page 0x%llX", (unsigned long long)page);
            continue;
        }
        if (!IsWritableProtection(memory.Protect)) {
            const DWORD writableProtection = IsExecutableProtection(memory.Protect)
                ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
            DWORD originalProtection = 0;
            if (!VirtualProtectEx(hProcess, reinterpret_cast<void*>(page), pageSize,
                                  writableProtection, &originalProtection)) {
                Log("PrepareIAT: VirtualProtectEx failed for 0x%llX (error %lu)",
                    (unsigned long long)page, GetLastError());
                continue;
            }
            gPreparedIatPages.push_back({processId, reinterpret_cast<void*>(page),
                                         pageSize, originalProtection});
            ++prepared;
        }

        // Changing a mapped image page to PAGE_READWRITE does not itself make a private copy.
        // Hyperion can therefore restore the shared IAT mapping before the external installer
        // writes its detours.  Touch one byte while the early write window is open so Windows
        // materializes a copy-on-write page owned by this Player process.  Writing the identical
        // byte preserves every import value while making later data-pointer changes independent
        // from the signed image mapping.
        BYTE unchanged = 0;
        SIZE_T transferred = 0;
        if (!ReadProcessMemory(hProcess, reinterpret_cast<const void*>(page),
                               &unchanged, sizeof(unchanged), &transferred) ||
            transferred != sizeof(unchanged)) {
            Log("PrepareIAT: cannot read COW trigger byte at 0x%llX (error %lu)",
                (unsigned long long)page, GetLastError());
            continue;
        }
        transferred = 0;
        if (!WriteProcessMemory(hProcess, reinterpret_cast<void*>(page),
                                &unchanged, sizeof(unchanged), &transferred) ||
            transferred != sizeof(unchanged)) {
            Log("PrepareIAT: cannot privatize page 0x%llX (error %lu)",
                (unsigned long long)page, GetLastError());
            continue;
        }
        ++privatized;
        ++writable;
    }

    Log("PrepareIAT: %zu/%zu page(s) writable and %zu privatized from RVA 0x%X "
        "for external hooks (%zu protection changes)",
        writable, pageCount, privatized, iat.VirtualAddress, prepared);
    return writable == pageCount && privatized == pageCount;
}

bool PrepareAddressForExternalHook(HANDLE hProcess, uintptr_t address, const char* label) {
    SYSTEM_INFO systemInfo = {};
    GetSystemInfo(&systemInfo);
    const uintptr_t pageSize = systemInfo.dwPageSize;
    const uintptr_t page = address & ~(pageSize - 1);
    const DWORD processId = GetProcessId(hProcess);

    MEMORY_BASIC_INFORMATION memory = {};
    if (VirtualQueryEx(hProcess, reinterpret_cast<const void*>(address),
                       &memory, sizeof(memory)) != sizeof(memory) ||
        memory.State != MEM_COMMIT || (memory.Protect & (PAGE_GUARD | PAGE_NOACCESS))) {
        Log("PrepareAddress: cannot query %s page 0x%llX", label,
            (unsigned long long)page);
        return false;
    }

    if (!IsWritableProtection(memory.Protect)) {
        const DWORD writableProtection = IsExecutableProtection(memory.Protect)
            ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
        DWORD originalProtection = 0;
        if (!VirtualProtectEx(hProcess, reinterpret_cast<void*>(page), pageSize,
                              writableProtection, &originalProtection)) {
            Log("PrepareAddress: VirtualProtectEx failed for %s page 0x%llX (error %lu)",
                label, (unsigned long long)page, GetLastError());
            return false;
        }
        gPreparedIatPages.push_back({processId, reinterpret_cast<void*>(page),
                                     pageSize, originalProtection});
    }

    // Touch the target byte itself while writes are still permitted. This creates the private
    // copy that the later resolved-pointer update needs without changing the lazy-import marker.
    BYTE unchanged = 0;
    SIZE_T transferred = 0;
    if (!ReadProcessMemory(hProcess, reinterpret_cast<const void*>(address),
                           &unchanged, sizeof(unchanged), &transferred) ||
        transferred != sizeof(unchanged)) {
        Log("PrepareAddress: cannot read %s byte at 0x%llX (error %lu)", label,
            (unsigned long long)address, GetLastError());
        return false;
    }
    transferred = 0;
    if (!WriteProcessMemory(hProcess, reinterpret_cast<void*>(address),
                            &unchanged, sizeof(unchanged), &transferred) ||
        transferred != sizeof(unchanged)) {
        Log("PrepareAddress: cannot privatize %s page 0x%llX (error %lu)", label,
            (unsigned long long)page, GetLastError());
        return false;
    }

    Log("PrepareAddress: %s page 0x%llX writable/private for slot 0x%llX",
        label, (unsigned long long)page, (unsigned long long)address);
    return true;
}

void RestorePreparedIatProtection(HANDLE hProcess) {
    const DWORD processId = GetProcessId(hProcess);
    size_t restored = 0;
    for (auto it = gPreparedIatPages.rbegin(); it != gPreparedIatPages.rend(); ++it) {
        if (it->processId != processId)
            continue;
        DWORD ignored = 0;
        if (VirtualProtectEx(hProcess, it->address, it->size,
                             it->originalProtection, &ignored))
            ++restored;
        else
            Log("RestoreIAT: VirtualProtectEx failed for 0x%llX (error %lu)",
                (unsigned long long)reinterpret_cast<uintptr_t>(it->address), GetLastError());
    }
    if (!gPreparedIatPages.empty())
        Log("RestoreIAT: restored %zu/%zu prepared page(s)", restored, gPreparedIatPages.size());
    gPreparedIatPages.clear();
}

// --- Version detection ---

RobloxEra DetectEra(const wchar_t* exePath) {
    DWORD handle = 0;
    DWORD size = GetFileVersionInfoSizeW(exePath, &handle);
    if (size == 0) return RobloxEra::Unknown;
    std::vector<char> buf(size);
    if (!GetFileVersionInfoW(exePath, handle, size, buf.data())) return RobloxEra::Unknown;
    
    char subBlock[128];
    snprintf(subBlock, sizeof(subBlock), "\\StringFileInfo\\040904E4\\ProductVersion");
    LPBYTE verBuf = nullptr; UINT verLen = 0;
    bool ok = VerQueryValueA(buf.data(), subBlock, (LPVOID*)&verBuf, &verLen);
    if (!ok) { snprintf(subBlock, sizeof(subBlock), "\\StringFileInfo\\000004B0\\ProductVersion"); ok = VerQueryValueA(buf.data(), subBlock, (LPVOID*)&verBuf, &verLen); }
    if (!ok) {
        struct LangAndCodePage { WORD lang; WORD codePage; }* translations = nullptr; UINT transLen = 0;
        if (VerQueryValueA(buf.data(), "\\VarFileInfo\\Translation", (LPVOID*)&translations, &transLen) && transLen >= sizeof(LangAndCodePage)) {
            snprintf(subBlock, sizeof(subBlock), "\\StringFileInfo\\%04X%04X\\ProductVersion", translations[0].lang, translations[0].codePage);
            ok = VerQueryValueA(buf.data(), subBlock, (LPVOID*)&verBuf, &verLen);
        }
    }
    if (!ok || !verBuf) return RobloxEra::Unknown;
    std::string ver(reinterpret_cast<char*>(verBuf), verLen - 1);
    size_t firstComma = ver.find(',');
    if (firstComma == std::string::npos) return RobloxEra::Unknown;
    size_t secondComma = ver.find(',', firstComma + 1);
    std::string eraStr = ver.substr(firstComma + 1, secondComma - firstComma - 1);
    while (!eraStr.empty() && eraStr.front() == ' ') eraStr.erase(0, 1);
    while (!eraStr.empty() && eraStr.back() == ' ') eraStr.pop_back();
    int eraNum = (int)strtol(eraStr.c_str(), nullptr, 10);
    if (eraNum == 719) return RobloxEra::Era719;
    // Hyperion offsets are build-specific. Never send an unknown modern build
    // through the nearest known patch table.
    if (eraNum >= 700) return RobloxEra::Unknown;
    if (eraNum >= 570) return RobloxEra::Era574;
    if (eraNum >= 460) return RobloxEra::Era463;
    return RobloxEra::Unknown;
}

// --- Remote module base ---

uintptr_t GetRemoteModuleBase(HANDLE hProcess, const wchar_t* moduleName) {
    HMODULE mods[1024]; DWORD needed = 0;
    if (EnumProcessModulesEx(hProcess, mods, sizeof(mods), &needed, LIST_MODULES_ALL)) {
        DWORD count = (std::min)(needed / static_cast<DWORD>(sizeof(HMODULE)),
                                 static_cast<DWORD>(_countof(mods)));
        for (DWORD i = 0; i < count; i++) {
            wchar_t name[MAX_PATH] = {0};
            if (!GetModuleBaseNameW(hProcess, mods[i], name, MAX_PATH)) continue;
            if (_wcsicmp(name, moduleName) == 0) return (uintptr_t)mods[i];
        }
    }

    // Hyperion can unlink system dependencies from the PEB lists while retaining their signed
    // SEC_IMAGE mappings. Roblox 0.728's native HTTP stack is already active when EnumProcessModules
    // still reports no Winsock image, so also enumerate mapped-image allocation bases by backing
    // path. This recovers the actual export base used by the engine's cached dispatch pointers;
    // loading a second WS2_32 copy produces different addresses and can never match those caches.
    SYSTEM_INFO systemInfo = {};
    GetSystemInfo(&systemInfo);
    uintptr_t cursor = reinterpret_cast<uintptr_t>(systemInfo.lpMinimumApplicationAddress);
    const uintptr_t maximum = reinterpret_cast<uintptr_t>(systemInfo.lpMaximumApplicationAddress);
    while (cursor < maximum) {
        MEMORY_BASIC_INFORMATION memory = {};
        if (VirtualQueryEx(hProcess, reinterpret_cast<const void*>(cursor),
                           &memory, sizeof(memory)) != sizeof(memory))
            break;

        const uintptr_t regionBase = reinterpret_cast<uintptr_t>(memory.BaseAddress);
        const uintptr_t regionEnd = regionBase + memory.RegionSize;
        if (memory.State == MEM_COMMIT && memory.Type == MEM_IMAGE &&
            memory.AllocationBase == memory.BaseAddress) {
            wchar_t mappedPath[32768] = {};
            if (GetMappedFileNameW(hProcess, memory.AllocationBase, mappedPath,
                                   static_cast<DWORD>(_countof(mappedPath)))) {
                const wchar_t* mappedName = wcsrchr(mappedPath, L'\\');
                mappedName = mappedName ? mappedName + 1 : mappedPath;
                if (_wcsicmp(mappedName, moduleName) == 0) {
                    Log("Recovered unlisted image %ls at 0x%llX through its mapped path",
                        moduleName,
                        (unsigned long long)reinterpret_cast<uintptr_t>(memory.AllocationBase));
                    return reinterpret_cast<uintptr_t>(memory.AllocationBase);
                }
            }
        }

        if (regionEnd <= cursor)
            break;
        cursor = regionEnd;
    }
    return 0;
}

uintptr_t GetRemoteModuleBase(DWORD pid, const wchar_t* moduleName) {
    HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!h) return 0;
    uintptr_t base = 0;
    for (int i = 0; i < 300; i++) { base = GetRemoteModuleBase(h, moduleName); if (base) break; Sleep(50); }
    CloseHandle(h);
    return base;
}

// --- Suspend/resume all threads ---

static void ForEachThread(DWORD pid, bool suspend) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snap == INVALID_HANDLE_VALUE) return;
    THREADENTRY32 te = { sizeof(THREADENTRY32) };
    if (Thread32First(snap, &te)) {
        DWORD myTid = GetCurrentThreadId();
        do {
            if (te.th32OwnerProcessID != pid) continue;
            if (te.th32ThreadID == myTid) continue;
            HANDLE th = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
            if (!th) continue;
            if (suspend) SuspendThread(th); else ResumeThread(th);
            CloseHandle(th);
        } while (Thread32Next(snap, &te));
    }
    CloseHandle(snap);
}

void SuspendProcess(DWORD pid) { ForEachThread(pid, true); }
void ResumeProcess(DWORD pid)  { ForEachThread(pid, false); }

// --- Section remap (temp file backed) ---

static bool WaitForStandaloneSectionView(HANDLE hProcess, uintptr_t moduleBase,
                                         const char* sectionName,
                                         DWORD timeoutMilliseconds) {
    IMAGE_DOS_HEADER dos = {};
    if (!ReadProcessMemory(hProcess, reinterpret_cast<const void*>(moduleBase),
                           &dos, sizeof(dos), nullptr) ||
        dos.e_magic != IMAGE_DOS_SIGNATURE)
        return false;

    IMAGE_NT_HEADERS64 nt = {};
    if (!ReadProcessMemory(hProcess,
                           reinterpret_cast<const void*>(moduleBase + dos.e_lfanew),
                           &nt, sizeof(nt), nullptr) ||
        nt.Signature != IMAGE_NT_SIGNATURE)
        return false;

    DWORD sectionRva = 0;
    const DWORD sectionOffset = dos.e_lfanew + sizeof(IMAGE_NT_HEADERS64);
    for (WORD i = 0; i < nt.FileHeader.NumberOfSections; ++i) {
        IMAGE_SECTION_HEADER section = {};
        if (!ReadProcessMemory(hProcess,
                               reinterpret_cast<const void*>(moduleBase + sectionOffset +
                                                              i * sizeof(section)),
                               &section, sizeof(section), nullptr))
            continue;
        char name[9] = {};
        memcpy(name, section.Name, 8);
        if (strcmp(name, sectionName) == 0) {
            sectionRva = section.VirtualAddress;
            break;
        }
    }
    if (!sectionRva) {
        Log("WaitForStandaloneSectionView: section \"%s\" not found", sectionName);
        return false;
    }

    const uintptr_t sectionAddress = moduleBase + sectionRva;
    // Wait until the independent view has stayed unchanged for a short settling window, then let the caller stop
    // the process and perform a simple private remap.
    const DWORD pollMilliseconds = 10;
    const DWORD stableMilliseconds = 500;
    const DWORD stablePollsRequired = stableMilliseconds / pollMilliseconds;
    const DWORD pollCount = (timeoutMilliseconds + pollMilliseconds - 1) /
                            pollMilliseconds;

    Log("Waiting for %s at 0x%llX to become a standalone mapped view",
        sectionName, (unsigned long long)sectionAddress);
    uintptr_t stableAllocationBase = 0;
    SIZE_T stableRegionSize = 0;
    DWORD stableType = 0;
    DWORD stableProtect = 0;
    DWORD stablePolls = 0;
    for (DWORD poll = 0; poll < pollCount; ++poll) {
        DWORD exitCode = 0;
        if (!GetExitCodeProcess(hProcess, &exitCode) || exitCode != STILL_ACTIVE) {
            Log("Target exited while waiting for standalone %s view (status 0x%08lX)",
                sectionName, exitCode);
            return false;
        }

        MEMORY_BASIC_INFORMATION memory = {};
        if (VirtualQueryEx(hProcess, reinterpret_cast<const void*>(sectionAddress),
                           &memory, sizeof(memory)) == sizeof(memory) &&
            reinterpret_cast<uintptr_t>(memory.AllocationBase) == sectionAddress &&
            memory.State == MEM_COMMIT &&
            (memory.Type == MEM_MAPPED || memory.Type == MEM_IMAGE)) {
            const uintptr_t allocationBase =
                reinterpret_cast<uintptr_t>(memory.AllocationBase);
            if (allocationBase == stableAllocationBase &&
                memory.RegionSize == stableRegionSize &&
                memory.Type == stableType && memory.Protect == stableProtect) {
                ++stablePolls;
            } else {
                stableAllocationBase = allocationBase;
                stableRegionSize = memory.RegionSize;
                stableType = memory.Type;
                stableProtect = memory.Protect;
                stablePolls = 1;
                Log("%s independent view observed after %lu ms (span 0x%zX, type 0x%lX, protect 0x%lX)",
                    sectionName, poll * pollMilliseconds,
                    static_cast<size_t>(memory.RegionSize), memory.Type,
                    memory.Protect);
            }
            if (stablePolls >= stablePollsRequired) {
                Log("%s independent view stable for %lu ms",
                    sectionName, stableMilliseconds);
                return true;
            }
        } else {
            stablePolls = 0;
        }

        Sleep(pollMilliseconds);
    }

    Log("Timed out after %lu ms waiting for standalone %s view",
        timeoutMilliseconds, sectionName);
    return false;
}

static uintptr_t RemapSection(HANDLE hProcess, uintptr_t moduleBase, const char* sectionName) {
    // Read PE headers
    IMAGE_DOS_HEADER dos = {};
    if (!ReadProcessMemory(hProcess, (LPCVOID)moduleBase, &dos, sizeof(dos), nullptr) || dos.e_magic != IMAGE_DOS_SIGNATURE) return 0;
    IMAGE_NT_HEADERS64 nt = {};
    if (!ReadProcessMemory(hProcess, (LPCVOID)(moduleBase + dos.e_lfanew), &nt, sizeof(nt), nullptr) || nt.Signature != IMAGE_NT_SIGNATURE) return 0;
    
    DWORD sectionRva = 0, sectionSize = 0;
    {
        DWORD sectionOff = dos.e_lfanew + sizeof(IMAGE_NT_HEADERS64);
        for (WORD i = 0; i < nt.FileHeader.NumberOfSections; i++) {
            IMAGE_SECTION_HEADER sec = {};
            if (!ReadProcessMemory(hProcess, (LPCVOID)(moduleBase + sectionOff + i * sizeof(sec)), &sec, sizeof(sec), nullptr)) continue;
            char name[9] = {0}; memcpy(name, sec.Name, 8);
            if (strcmp(name, sectionName) == 0) { sectionRva = sec.VirtualAddress; sectionSize = sec.Misc.VirtualSize; break; }
        }
    }
    if (!sectionRva || !sectionSize) { Log("RemapSection: section \"%s\" not found", sectionName); return 0; }
    uintptr_t sectionAddr = moduleBase + sectionRva;
    const DWORD sectionAlignment = nt.OptionalHeader.SectionAlignment;
    const size_t alignedSectionSize = sectionAlignment
        ? (static_cast<size_t>(sectionSize) + sectionAlignment - 1) &
              ~(static_cast<size_t>(sectionAlignment) - 1)
        : sectionSize;
    SYSTEM_INFO systemInfo = {};
    GetSystemInfo(&systemInfo);
    const size_t pageSize = systemInfo.dwPageSize;
    const size_t pageAlignedSectionSize = pageSize
        ? (static_cast<size_t>(sectionSize) + pageSize - 1) & ~(pageSize - 1)
        : sectionSize;
    Log("RemapSection: %s at 0x%llX (virtual 0x%X, page span 0x%zX, PE aligned 0x%zX)",
        sectionName, (unsigned long long)sectionAddr, sectionSize,
        pageAlignedSectionSize, alignedSectionSize);

    // Discover the complete standalone view instead of assuming that PE
    // SectionAlignment describes the view length. 0.728's independently mapped
    // .byfron view is page-aligned (0x12C4000); the remaining gap before .rdata
    // is intentionally unmapped. Retain every region's original protection.
    struct RegionSnapshot {
        size_t offset;
        size_t size;
        DWORD state;
        DWORD protect;
    };
    std::vector<RegionSnapshot> regions;

    MEMORY_BASIC_INFORMATION first = {};
    if (VirtualQueryEx(hProcess, reinterpret_cast<const void*>(sectionAddr),
                       &first, sizeof(first)) != sizeof(first)) {
        Log("RemapSection: VirtualQueryEx failed at view start (error %lu)", GetLastError());
        return 0;
    }
    const uintptr_t allocationBase = reinterpret_cast<uintptr_t>(first.AllocationBase);
    const uintptr_t imageEnd = moduleBase + nt.OptionalHeader.SizeOfImage;
    uintptr_t cursor = sectionAddr;
    uintptr_t viewEnd = sectionAddr;
    while (cursor < imageEnd) {
        MEMORY_BASIC_INFORMATION memory = {};
        if (VirtualQueryEx(hProcess, reinterpret_cast<const void*>(cursor),
                           &memory, sizeof(memory)) != sizeof(memory) ||
            reinterpret_cast<uintptr_t>(memory.AllocationBase) != allocationBase)
            break;

        const uintptr_t regionStart = reinterpret_cast<uintptr_t>(memory.BaseAddress);
        const uintptr_t regionEnd = regionStart + memory.RegionSize;
        const uintptr_t clippedStart = (std::max)(regionStart, sectionAddr);
        const uintptr_t clippedEnd = (std::min)(regionEnd, imageEnd);
        if (clippedEnd > clippedStart) {
            regions.push_back({static_cast<size_t>(clippedStart - sectionAddr),
                               static_cast<size_t>(clippedEnd - clippedStart),
                               memory.State, memory.Protect});
            viewEnd = clippedEnd;
        }
        if (regionEnd <= cursor)
            break;
        cursor = regionEnd;
    }

    const size_t viewSize = static_cast<size_t>(viewEnd - sectionAddr);
    if (allocationBase != sectionAddr || viewSize < pageAlignedSectionSize) {
        Log("RemapSection: incomplete standalone view (allocation=0x%llX, span=0x%zX, expected>=0x%zX)",
            (unsigned long long)allocationBase, viewSize, pageAlignedSectionSize);
        return 0;
    }
    Log("RemapSection: standalone view span 0x%zX across %zu region(s)",
        viewSize, regions.size());

    std::vector<uint8_t> sectionData(viewSize, 0);
    size_t readableBytes = 0;
    for (const auto& region : regions) {
        if (region.state != MEM_COMMIT ||
            (region.protect & (PAGE_NOACCESS | PAGE_GUARD)))
            continue;
        SIZE_T read = 0;
        if (!ReadProcessMemory(hProcess,
                               reinterpret_cast<const void*>(sectionAddr + region.offset),
                               sectionData.data() + region.offset, region.size, &read) ||
            read != region.size) {
            Log("RemapSection: ReadProcessMemory failed at +0x%zX (error %lu)",
                region.offset, GetLastError());
            return 0;
        }
        readableBytes += read;
    }
    Log("RemapSection: captured 0x%zX readable bytes", readableBytes);
    
    // Private memory: NtUnmapViewOfSection + VirtualAllocEx + write back
    typedef NTSTATUS (NTAPI* NtUnmapViewOfSection_t)(HANDLE, PVOID);
    auto pNtUnmapView = (NtUnmapViewOfSection_t)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtUnmapViewOfSection");
    if (!pNtUnmapView) return 0;
    NTSTATUS st = pNtUnmapView(hProcess, (PVOID)sectionAddr);
    if (st < 0) { Log("RemapSection: NtUnmapViewOfSection failed 0x%lX", (unsigned long)st); return 0; }
    Log("RemapSection: unmapped old view at 0x%llX", (unsigned long long)sectionAddr);
    
    LPVOID newBase = VirtualAllocEx(hProcess, (LPVOID)sectionAddr, viewSize,
                                    MEM_RESERVE, PAGE_NOACCESS);
    if (!newBase || newBase != (LPVOID)sectionAddr) { Log("RemapSection: VirtualAllocEx failed"); return 0; }
    Log("RemapSection: reserved private view at 0x%llX (span 0x%zX)",
        (unsigned long long)newBase, viewSize);

    auto privateProtection = [](DWORD protection) {
        const DWORD modifiers = protection & (PAGE_GUARD | PAGE_NOCACHE | PAGE_WRITECOMBINE);
        switch (protection & 0xFF) {
        case PAGE_WRITECOPY: return PAGE_READWRITE | modifiers;
        case PAGE_EXECUTE_WRITECOPY: return PAGE_EXECUTE_READWRITE | modifiers;
        default: return (protection & 0xFF) | modifiers;
        }
    };

    for (const auto& region : regions) {
        if (region.state != MEM_COMMIT)
            continue;
        void* regionAddress = reinterpret_cast<void*>(sectionAddr + region.offset);
        if (!VirtualAllocEx(hProcess, regionAddress, region.size, MEM_COMMIT, PAGE_READWRITE)) {
            Log("RemapSection: commit failed at +0x%zX (error %lu)",
                region.offset, GetLastError());
            return 0;
        }
        SIZE_T written = 0;
        if (!WriteProcessMemory(hProcess, regionAddress,
                                sectionData.data() + region.offset,
                                region.size, &written) || written != region.size) {
            Log("RemapSection: write failed at +0x%zX (error %lu)",
                region.offset, GetLastError());
            return 0;
        }
        DWORD oldProtection = 0;
        const DWORD restoredProtection = privateProtection(region.protect);
        if (!VirtualProtectEx(hProcess, regionAddress, region.size,
                              restoredProtection, &oldProtection)) {
            Log("RemapSection: protection restore failed at +0x%zX (0x%lX, error %lu)",
                region.offset, restoredProtection, GetLastError());
            return 0;
        }
    }
    FlushInstructionCache(hProcess, reinterpret_cast<const void*>(sectionAddr), viewSize);
    Log("RemapSection: %s remapped as private memory", sectionName);
    return (uintptr_t)newBase;
}

// --- Unhook ntdll functions ---

void UnhookNtFunctions(HANDLE hProcess, DWORD pid, uintptr_t byfronModuleBase) {
    uintptr_t byfronBase = byfronModuleBase;
    if (!byfronBase) byfronBase = FindModuleWithSection(pid, ".byfron");
    if (!byfronBase) { Log("UnhookNt: cannot find .byfron"); return; }
    
    uintptr_t byfronEnd = 0;
    HANDLE hQ = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hQ) { MODULEINFO mi = {}; if (GetModuleInformation(hQ, (HMODULE)byfronBase, &mi, sizeof(mi))) byfronEnd = byfronBase + mi.SizeOfImage; CloseHandle(hQ); }
    Log("UnhookNt: .byfron at 0x%llX - 0x%llX", (unsigned long long)byfronBase, (unsigned long long)byfronEnd);
    
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (!hNtdll) return;
    auto* dos = (PIMAGE_DOS_HEADER)hNtdll;
    auto* nth = (PIMAGE_NT_HEADERS)((BYTE*)hNtdll + dos->e_lfanew);
    auto& expDir = nth->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (!expDir.VirtualAddress) return;
    auto* exports = (PIMAGE_EXPORT_DIRECTORY)((BYTE*)hNtdll + expDir.VirtualAddress);
    DWORD* nameRVAs = (DWORD*)((BYTE*)hNtdll + exports->AddressOfNames);
    WORD*  ordRVAs  = (WORD*)((BYTE*)hNtdll + exports->AddressOfNameOrdinals);
    DWORD* funcRVAs = (DWORD*)((BYTE*)hNtdll + exports->AddressOfFunctions);
    
    Log("UnhookNt: scanning %lu ntdll exports", exports->NumberOfNames);
    int restored = 0;
    for (DWORD i = 0; i < exports->NumberOfNames; i++) {
        FARPROC fnAddr = (FARPROC)((BYTE*)hNtdll + funcRVAs[ordRVAs[i]]);
        uint8_t remoteBytes[5] = {};
        if (!ReadProcessMemory(hProcess, fnAddr, remoteBytes, 5, nullptr)) continue;
        if (remoteBytes[0] != 0xE9) continue;
        int32_t rel32 = *(int32_t*)(remoteBytes + 1);
        uintptr_t target = (uintptr_t)fnAddr + 5 + rel32;
        if (!byfronEnd || target < byfronBase || target >= byfronEnd) continue;
        
        uint8_t origBytes[5]; memcpy(origBytes, fnAddr, 5);
        const char* name = (const char*)((BYTE*)hNtdll + nameRVAs[i]);
        DWORD oldProt = 0;
        if (VirtualProtectEx(hProcess, (LPVOID)fnAddr, 5, PAGE_EXECUTE_READWRITE, &oldProt)) {
            SIZE_T w = 0;
            if (WriteProcessMemory(hProcess, (LPVOID)fnAddr, origBytes, 5, &w) && w == 5) { Log("UnhookNt: restored %s", name); restored++; }
            DWORD ign = 0; VirtualProtectEx(hProcess, (LPVOID)fnAddr, 5, oldProt, &ign);
        }
    }
    Log("UnhookNt: restored %d hooked ntdll functions", restored);

    // Direct unhook of LdrLoadDll — compare remote with disk bytes
    {
        HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
        if (hNtdll) {
            FARPROC pRemoteLdr = GetProcAddress(hNtdll, "LdrLoadDll");
            if (pRemoteLdr) {
                uint8_t remoteBytes[64] = {};
                uint8_t diskBytes[64] = {};

                // Read from target process
                ReadProcessMemory(hProcess, pRemoteLdr, remoteBytes, sizeof(remoteBytes), nullptr);

                // Read from CLEAN disk ntdll
                {
                    wchar_t sysPath[MAX_PATH];
                    GetSystemDirectoryW(sysPath, MAX_PATH);
                    wcscat_s(sysPath, L"\\ntdll.dll");
                    HANDLE hFile = CreateFileW(sysPath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
                    if (hFile != INVALID_HANDLE_VALUE) {
                        HANDLE hMap = CreateFileMappingW(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
                        if (hMap) {
                            LPVOID view = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
                            if (view) {
                                auto* ddos = (PIMAGE_DOS_HEADER)view;
                                auto* dnth = (PIMAGE_NT_HEADERS)((BYTE*)view + ddos->e_lfanew);
                                auto* dexpDir = &dnth->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
                                auto* dexports = (PIMAGE_EXPORT_DIRECTORY)((BYTE*)view + dexpDir->VirtualAddress);
                                DWORD* dnameRVAs = (DWORD*)((BYTE*)view + dexports->AddressOfNames);
                                WORD*  dordRVAs  = (WORD*)((BYTE*)view + dexports->AddressOfNameOrdinals);
                                DWORD* dfuncRVAs = (DWORD*)((BYTE*)view + dexports->AddressOfFunctions);
                                for (DWORD i = 0; i < dexports->NumberOfNames; i++) {
                                    const char* name = (const char*)((BYTE*)view + dnameRVAs[i]);
                                    if (strcmp(name, "LdrLoadDll") == 0) {
                                        void* diskAddr = (BYTE*)view + dfuncRVAs[dordRVAs[i]];
                                        memcpy(diskBytes, diskAddr, sizeof(diskBytes));
                                        break;
                                    }
                                }
                            }
                            if (view) UnmapViewOfFile(view);
                        }
                        if (hMap) CloseHandle(hMap);
                        CloseHandle(hFile);
                    }
                }

                if (memcmp(remoteBytes, diskBytes, sizeof(diskBytes)) != 0) {
                    Log("UnhookNt: LdrLoadDll is HOOKED, restoring %zu bytes", sizeof(diskBytes));
                    DWORD oldProt;
                    VirtualProtectEx(hProcess, pRemoteLdr, sizeof(diskBytes), PAGE_EXECUTE_READWRITE, &oldProt);
                    SIZE_T w;
                    WriteProcessMemory(hProcess, pRemoteLdr, diskBytes, sizeof(diskBytes), &w);
                    VirtualProtectEx(hProcess, pRemoteLdr, sizeof(diskBytes), oldProt, &oldProt);
                    Log("UnhookNt: LdrLoadDll restored");
                } else {
                    Log("UnhookNt: LdrLoadDll not hooked (matches disk)");
                }
            }
        }
    }
}

// --- Find module with section ---

uintptr_t FindModuleWithSection(DWORD pid, const char* sectionName) {
    HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!h) return 0;
    HMODULE mods[1024]; DWORD needed = 0;
    if (!EnumProcessModulesEx(h, mods, sizeof(mods), &needed, LIST_MODULES_ALL)) { CloseHandle(h); return 0; }
    DWORD count = needed / sizeof(HMODULE);
    for (DWORD i = 0; i < count; i++) {
        IMAGE_DOS_HEADER dos = {};
        if (!ReadProcessMemory(h, mods[i], &dos, sizeof(dos), nullptr) || dos.e_magic != IMAGE_DOS_SIGNATURE) continue;
        IMAGE_NT_HEADERS64 nt = {};
        if (!ReadProcessMemory(h, (LPCVOID)((uintptr_t)mods[i] + dos.e_lfanew), &nt, sizeof(nt), nullptr) || nt.Signature != IMAGE_NT_SIGNATURE) continue;
        DWORD secOff = dos.e_lfanew + sizeof(IMAGE_NT_HEADERS64);
        for (WORD j = 0; j < nt.FileHeader.NumberOfSections; j++) {
            IMAGE_SECTION_HEADER sec = {};
            if (!ReadProcessMemory(h, (LPCVOID)((uintptr_t)mods[i] + secOff + j * sizeof(sec)), &sec, sizeof(sec), nullptr)) continue;
            char nm[9] = {0}; memcpy(nm, sec.Name, 8);
            if (strcmp(nm, sectionName) == 0) { CloseHandle(h); return (uintptr_t)mods[i]; }
        }
    }
    CloseHandle(h); return 0;
}

bool HasSectionOnDisk(const wchar_t* exePath, const char* sectionName) {
    HANDLE hFile = CreateFileW(exePath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return false;
    HANDLE hMapping = CreateFileMappingW(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!hMapping) { CloseHandle(hFile); return false; }
    LPVOID view = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
    if (!view) { CloseHandle(hMapping); CloseHandle(hFile); return false; }
    bool found = false;
    auto* dos = (IMAGE_DOS_HEADER*)view;
    if (dos->e_magic == IMAGE_DOS_SIGNATURE) {
        auto* nt = (IMAGE_NT_HEADERS64*)((uintptr_t)view + dos->e_lfanew);
        if (nt->Signature == IMAGE_NT_SIGNATURE) {
            auto* sec = IMAGE_FIRST_SECTION(nt);
            for (WORD i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++) {
                char nm[9] = {0}; memcpy(nm, sec->Name, 8);
                if (strcmp(nm, sectionName) == 0) { found = true; break; }
            }
        }
    }
    UnmapViewOfFile(view); CloseHandle(hMapping); CloseHandle(hFile);
    return found;
}

static bool _byfronSectionInModule(DWORD pid, uintptr_t base) {
    HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!h) return false;
    IMAGE_DOS_HEADER dos = {};
    bool ok = ReadProcessMemory(h, (LPCVOID)base, &dos, sizeof(dos), nullptr) && dos.e_magic == IMAGE_DOS_SIGNATURE;
    if (ok) {
        IMAGE_NT_HEADERS64 nth = {};
        ok = ReadProcessMemory(h, (LPCVOID)(base + dos.e_lfanew), &nth, sizeof(nth), nullptr) && nth.Signature == IMAGE_NT_SIGNATURE;
        if (ok) {
            DWORD secOff = dos.e_lfanew + sizeof(IMAGE_NT_HEADERS64);
            ok = false;
            for (WORD s = 0; s < nth.FileHeader.NumberOfSections; s++) {
                IMAGE_SECTION_HEADER sec = {};
                if (ReadProcessMemory(h, (LPCVOID)(base + secOff + s * sizeof(sec)), &sec, sizeof(sec), nullptr)) {
                    char nm[9] = {0}; memcpy(nm, sec.Name, 8);
                    if (strcmp(nm, ".byfron") == 0) { ok = true; break; }
                }
            }
        }
    }
    CloseHandle(h);
    return ok;
}

// --- Main bypass ---

bool BypassHyperion(HANDLE hProcess, DWORD pid, const wchar_t* exePath) {
    gLog = stdout;
    gPreparedIatPages.clear();
    RobloxEra era = DetectEra(exePath);
    if (!NeedsBypass(era)) { Log("Era %ls does not need bypass", EraName(era)); return true; }
    // Player 0.719 is handled before its first instruction by the image-backed,
    // loader-free WSP bridge. It deliberately has no Hyperion code-page patch
    // table and never enters the legacy remap/injection pipeline below.
    if (UsesEarlyWspRedirect(era)) {
        Log("Era %ls uses the clean early WSP path; no Hyperion code patches", EraName(era));
        return true;
    }
    Log("Era %ls requires Hyperion bypass", EraName(era));
    
    auto patches = GetPatchesForEra(era);
    if (patches.empty()) { Log("No patches for era %ls", EraName(era)); return false; }
    
    // 1. Find .byfron module (prefer DLL over EXE decoy). For 0.728 the
    // injector has already waited for the real DLL, so avoid another broad
    // module scan while its startup remap guard is racing us.
    uintptr_t hyperionBase = 0;
    Log("Searching for .byfron module...");
    for (int i = 0; i < 300; i++) {
        hyperionBase = FindModuleWithSection(pid, ".byfron");
        if (hyperionBase)
            break;
        Sleep(50);
    }
    if (!hyperionBase) { Log("Timed out waiting for .byfron"); return false; }
    
    // Prefer RobloxPlayerBeta.dll
    {
        wchar_t modName[MAX_PATH] = {0};
        HANDLE hQ = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (hQ) { GetModuleBaseNameW(hQ, (HMODULE)hyperionBase, modName, MAX_PATH); CloseHandle(hQ); }
        Log(".byfron found in module: %ls (base 0x%llX)", modName, (unsigned long long)hyperionBase);
        if (_wcsicmp(modName, L"RobloxPlayerBeta.exe") == 0) {
            uintptr_t dllBase = GetRemoteModuleBase(pid, L"RobloxPlayerBeta.dll");
            if (dllBase && _byfronSectionInModule(pid, dllBase)) { hyperionBase = dllBase; Log("using RobloxPlayerBeta.dll (0x%llX) instead", (unsigned long long)hyperionBase); }
        }
    }

    // Match the supplied external injectors: attach only after initialization
    // has settled, then stop every thread for the remap and patch writes.
    SuspendProcess(pid);
    Log("Process suspended");
    
    // 3. Remap .byfron
    const uintptr_t remapped = RemapSection(hProcess, hyperionBase, ".byfron");
    if (!remapped)
        Log("RemapSection failed — continuing");
    
    if (!remapped) {
        ResumeProcess(pid);
        return false;
    }

    if (!PrepareIatForExternalHooks(hProcess, hyperionBase))
        Log("PrepareIAT failed - file-hook fallback may be unavailable");

    // The remapped view is now private, so the compatibility patches can be
    // applied without Hyperion restoring the original mapped pages.
    Log("Applying all %zu Hyperion patches", patches.size());
    size_t applied = 0, failed = 0;
    for (size_t p = 0; p < patches.size(); p++) {
        auto& pt = patches[p];
        uintptr_t addr = hyperionBase + pt.rva;
        DWORD oldProt = 0;
        VirtualProtectEx(hProcess, (LPVOID)addr, pt.bytes.size(), PAGE_EXECUTE_READWRITE, &oldProt);
        SIZE_T written = 0;
        if (!WriteProcessMemory(hProcess, (LPVOID)addr, pt.bytes.data(), pt.bytes.size(), &written)) {
            Log("Patch at 0x%llX failed (err %lu)", (unsigned long long)addr, GetLastError());
            failed++;
        } else {
            Log("Patched 0x%llX (%zu bytes)", (unsigned long long)addr, pt.bytes.size());
            applied++;
        }
        DWORD ignored = 0;
        VirtualProtectEx(hProcess, (LPVOID)addr, pt.bytes.size(), oldProt, &ignored);
    }
    Log("Patches applied: %zu ok, %zu failed", applied, failed);

    // The staged 0.729 profile restores Hyperion's ntdll trampolines. The
    // independently derived 0.728 profile does not require those writes, and
    // applying the 0.729-era operation to 0.728 makes the process disappear
    // before Roblox initializes its own logger.
    UnhookNtFunctions(hProcess, pid, hyperionBase);
    
    // 6. Resume without a debugger. Hyperion checks for debug ownership.
    ResumeProcess(pid);
    Log("Process resumed - Hyperion neutralised");

    // This ETW rewrite belongs to the staged 0.729 profile. Keep it out of the
    // 0.728 port: it writes shared ntdll code after the target has resumed and
    // is not needed by the verified 0.728 patch table.
    return true;
}

// --- Read hook info from remote DLL and write JMP hooks ---

struct HookEnableInfo {
    char   targetModule[32];
    char   targetFunc[64];
    void*  detourFunc;
    void** originalFuncStorage;
};

struct HookInfoBlock {
    uint64_t magic;
    HookEnableInfo table[16];
    int count;
};

bool WaitForHookInfoReady(HANDLE hProcess, uintptr_t dllBase,
                          const std::vector<uint8_t>& dllData, DWORD timeoutMs) {
    if (dllData.size() < sizeof(IMAGE_DOS_HEADER))
        return false;
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(dllData.data());
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0 ||
        static_cast<size_t>(dos->e_lfanew) + sizeof(IMAGE_NT_HEADERS64) > dllData.size())
        return false;

    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(dllData.data() + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        return false;
    const auto* sections = IMAGE_FIRST_SECTION(nt);
    const auto* sectionEnd = sections + nt->FileHeader.NumberOfSections;
    if (reinterpret_cast<const uint8_t*>(sectionEnd) > dllData.data() + dllData.size())
        return false;

    uintptr_t hookInfoAddress = 0;
    for (const IMAGE_SECTION_HEADER* section = sections; section != sectionEnd; ++section) {
        char name[9] = {};
        memcpy(name, section->Name, sizeof(section->Name));
        if (strcmp(name, ".hkinfo") == 0) {
            hookInfoAddress = dllBase + section->VirtualAddress;
            break;
        }
    }
    if (!hookInfoAddress)
        return false;

    constexpr uint64_t kHookInfoMagic = 0x4B4F4F484E4F4F42ULL;
    const ULONGLONG deadline = GetTickCount64() + timeoutMs;
    HookInfoBlock info = {};
    do {
        SIZE_T read = 0;
        if (ReadProcessMemory(hProcess, reinterpret_cast<const void*>(hookInfoAddress),
                              &info, sizeof(info), &read) && read == sizeof(info) &&
            info.magic == kHookInfoMagic && info.count > 0 && info.count <= 16) {
            printf("InstallHooks: hook table ready with %d entries\n", info.count);
            return true;
        }

        if (WaitForSingleObject(hProcess, 0) == WAIT_OBJECT_0)
            break;
        Sleep(10);
    } while (GetTickCount64() < deadline);

    printf("InstallHooks: hook table did not become ready within %lu ms (magic=0x%llX count=%d)\n",
           timeoutMs, (unsigned long long)info.magic, info.count);
    return false;
}

// Hyperion enables ProcessDynamicCodePolicy, which prevents changing some shared system-code
// pages (notably Kernel32 and ntdll) to executable-writable.  The hook trampoline is still valid --
// MinHook built it while noobHook initialized -- so redirect imports in Roblox-owned modules when
// the export-prologue write is refused. IAT pointers are data and do not require executable writes;
// PrepareIatForExternalHooks also handles Roblox image pages that reject late protection changes.
//
// This remains CFG-safe with the current manual mapper: AllocateImageNearSystemDlls commits the hook
// image as PAGE_EXECUTE_READWRITE without PAGE_TARGETS_INVALID. Windows consequently registers the
// committed executable pages as valid indirect-call targets, including the detours written below.
static bool ReadRemoteAscii(HANDLE hProcess, uintptr_t address, char* out, size_t outSize) {
    if (!address || !out || outSize < 2)
        return false;

    SIZE_T read = 0;
    // ReadProcessMemory reports FALSE if the requested range crosses a page boundary, but may still
    // return the useful prefix in `read` (which is all an ASCIIZ import name needs).
    ReadProcessMemory(hProcess, reinterpret_cast<const void*>(address), out, outSize - 1, &read);
    if (read == 0)
        return false;
    out[read] = '\0';
    if (void* end = memchr(out, '\0', read))
        *static_cast<char*>(end) = '\0';
    else
        out[outSize - 1] = '\0';
    return true;
}

static bool IsSystemModule(HANDLE hProcess, HMODULE module, wchar_t* moduleName, size_t moduleNameCount) {
    wchar_t modulePath[MAX_PATH] = {};
    if (!GetModuleFileNameExW(hProcess, module, modulePath, _countof(modulePath)))
        return true; // Do not write an IAT belonging to a module we cannot identify.
    if (!GetModuleBaseNameW(hProcess, module, moduleName, static_cast<DWORD>(moduleNameCount)))
        return true;

    wchar_t windowsDir[MAX_PATH] = {};
    UINT windowsDirLength = GetWindowsDirectoryW(windowsDir, _countof(windowsDir));
    if (windowsDirLength == 0 || windowsDirLength >= _countof(windowsDir))
        return true;
    while (windowsDirLength > 0 && (windowsDir[windowsDirLength - 1] == L'\\' || windowsDir[windowsDirLength - 1] == L'/'))
        --windowsDirLength;

    return _wcsnicmp(modulePath, windowsDir, windowsDirLength) == 0 &&
           (modulePath[windowsDirLength] == L'\\' || modulePath[windowsDirLength] == L'/');
}

enum class IatWriteResult {
    Failed,
    Installed,
    Inconsistent,
};

static IatWriteResult WriteRemoteIatPointer(HANDLE hProcess, uintptr_t slot, uintptr_t detour) {
    uintptr_t original = 0;
    SIZE_T read = 0;
    if (!ReadProcessMemory(hProcess, reinterpret_cast<const void*>(slot),
                           &original, sizeof(original), &read) || read != sizeof(original))
        return IatWriteResult::Failed;
    if (original == detour)
        return IatWriteResult::Installed;

    SYSTEM_INFO systemInfo = {};
    GetSystemInfo(&systemInfo);
    const uintptr_t pageMask = static_cast<uintptr_t>(systemInfo.dwPageSize - 1);
    void* protectBase = reinterpret_cast<void*>(slot & ~pageMask);
    SIZE_T protectSize = ((slot + sizeof(detour) + pageMask) & ~pageMask) -
                         reinterpret_cast<uintptr_t>(protectBase);

    MEMORY_BASIC_INFORMATION memory = {};
    const bool alreadyWritable =
        VirtualQueryEx(hProcess, reinterpret_cast<const void*>(slot),
                       &memory, sizeof(memory)) == sizeof(memory) &&
        memory.State == MEM_COMMIT && IsWritableProtection(memory.Protect) &&
        !(memory.Protect & (PAGE_GUARD | PAGE_NOACCESS));

    ULONG oldProtect = 0;
    if (!alreadyWritable) {
        NTSTATUS protectStatus = SysNtProtectVirtualMemory(
            hProcess, &protectBase, &protectSize, PAGE_READWRITE, &oldProtect);
        if (protectStatus < 0) {
            printf("InstallHooks: pointer slot 0x%llX protection change failed "
                   "(status=0x%08lX, state=0x%lX, protect=0x%lX, type=0x%lX)\n",
                   (unsigned long long)slot, static_cast<unsigned long>(protectStatus),
                   memory.State, memory.Protect, memory.Type);
            return IatWriteResult::Failed;
        }
    }

    SIZE_T written = 0;
    NTSTATUS writeStatus = SysNtWriteVirtualMemory(
        hProcess, reinterpret_cast<void*>(slot), &detour, sizeof(detour), &written);

    uintptr_t observed = 0;
    read = 0;
    const bool installed = writeStatus >= 0 && written == sizeof(detour) &&
        ReadProcessMemory(hProcess, reinterpret_cast<const void*>(slot),
                          &observed, sizeof(observed), &read) &&
        read == sizeof(observed) && observed == detour;
    if (!installed) {
        printf("InstallHooks: pointer slot 0x%llX write failed "
               "(status=0x%08lX, wrote=%zu, observed=0x%llX, expected=0x%llX)\n",
               (unsigned long long)slot, static_cast<unsigned long>(writeStatus), written,
               (unsigned long long)observed, (unsigned long long)detour);
    }
    bool rolledBack = true;
    if (!installed) {
        // An aligned pointer write should be all-or-nothing, but never leave an IAT slot partially
        // updated if a protection product produces a short write.
        SIZE_T restored = 0;
        NTSTATUS restoreStatus = SysNtWriteVirtualMemory(
            hProcess, reinterpret_cast<void*>(slot), &original, sizeof(original), &restored);
        observed = 0;
        read = 0;
        rolledBack = restoreStatus >= 0 && restored == sizeof(original) &&
            ReadProcessMemory(hProcess, reinterpret_cast<const void*>(slot),
                              &observed, sizeof(observed), &read) &&
            read == sizeof(observed) && observed == original;
    }

    if (!alreadyWritable) {
        ULONG ignoredProtect = 0;
        SysNtProtectVirtualMemory(hProcess, &protectBase, &protectSize, oldProtect, &ignoredProtect);
    }
    if (installed)
        return IatWriteResult::Installed;
    return rolledBack ? IatWriteResult::Failed : IatWriteResult::Inconsistent;
}

static size_t PatchRemoteThunkTable(HANDLE hProcess, uintptr_t moduleBase, DWORD imageSize,
                                    DWORD lookupRva, DWORD iatRva, const char* importedModule,
                                    const char* targetModule, const char* targetFunc,
                                    uintptr_t targetAddress, uintptr_t detourAddress,
                                    const wchar_t* ownerName) {
    if (!iatRva || iatRva >= imageSize)
        return 0;

    const size_t maxThunks = (std::min)(static_cast<size_t>((imageSize - iatRva) / sizeof(IMAGE_THUNK_DATA64)),
                                       static_cast<size_t>(65536));
    size_t patched = 0;
    for (size_t index = 0; index < maxThunks; ++index) {
        IMAGE_THUNK_DATA64 iatThunk = {};
        uintptr_t iatSlot = moduleBase + iatRva + index * sizeof(iatThunk);
        if (!ReadProcessMemory(hProcess, reinterpret_cast<const void*>(iatSlot),
                               &iatThunk, sizeof(iatThunk), nullptr) || iatThunk.u1.Function == 0)
            break;

        bool functionMatches = false;
        if (lookupRva && lookupRva < imageSize) {
            IMAGE_THUNK_DATA64 lookupThunk = {};
            uintptr_t lookupSlot = moduleBase + lookupRva + index * sizeof(lookupThunk);
            if (!ReadProcessMemory(hProcess, reinterpret_cast<const void*>(lookupSlot),
                                   &lookupThunk, sizeof(lookupThunk), nullptr) || lookupThunk.u1.AddressOfData == 0)
                break;
            if (!IMAGE_SNAP_BY_ORDINAL64(lookupThunk.u1.Ordinal) && lookupThunk.u1.AddressOfData < imageSize) {
                char importedName[128] = {};
                // IMAGE_IMPORT_BY_NAME starts with a two-byte hint followed by its ASCIIZ name.
                if (ReadRemoteAscii(hProcess, moduleBase + lookupThunk.u1.AddressOfData + sizeof(WORD),
                                    importedName, sizeof(importedName)))
                    functionMatches = strcmp(importedName, targetFunc) == 0;
            }
        } else {
            // Some bound images omit OriginalFirstThunk.  In that case the resolved slot is the
            // only trustworthy identity available.
            functionMatches = static_cast<uintptr_t>(iatThunk.u1.Function) == targetAddress;
        }

        const bool providerMatches = _stricmp(importedModule, targetModule) == 0 ||
                                     static_cast<uintptr_t>(iatThunk.u1.Function) == targetAddress;
        if (!functionMatches || !providerMatches)
            continue;
        if (static_cast<uintptr_t>(iatThunk.u1.Function) == detourAddress)
            continue; // A second installer pass must not count or rewrite an existing fallback.

        IatWriteResult writeResult = WriteRemoteIatPointer(hProcess, iatSlot, detourAddress);
        if (writeResult == IatWriteResult::Installed) {
            printf("InstallHooks: IAT fallback %ls!%s (%s) slot 0x%llX -> 0x%llX OK\n",
                   ownerName, targetFunc, importedModule, (unsigned long long)iatSlot,
                   (unsigned long long)detourAddress);
            ++patched;
        } else if (writeResult == IatWriteResult::Inconsistent) {
            printf("InstallHooks: IAT fallback %ls!%s slot 0x%llX could not be rolled back; aborting fallback\n",
                   ownerName, targetFunc, (unsigned long long)iatSlot);
            return (std::numeric_limits<size_t>::max)();
        } else {
            printf("InstallHooks: IAT fallback %ls!%s slot 0x%llX failed\n",
                   ownerName, targetFunc, (unsigned long long)iatSlot);
        }
    }
    return patched;
}

struct DelayImportDescriptor {
    DWORD attributes;
    DWORD name;
    DWORD moduleHandle;
    DWORD iat;
    DWORD intTable;
    DWORD boundIat;
    DWORD unloadIat;
    DWORD timeStamp;
};

static size_t PatchModuleIat(HANDLE hProcess, HMODULE module, const wchar_t* moduleName,
                             const char* targetModule, const char* targetFunc,
                             uintptr_t targetAddress, uintptr_t detourAddress) {
    const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(module);
    IMAGE_DOS_HEADER dos = {};
    if (!ReadProcessMemory(hProcess, module, &dos, sizeof(dos), nullptr) ||
        dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew <= 0 || dos.e_lfanew > 0x100000)
        return 0;

    IMAGE_NT_HEADERS64 nt = {};
    if (!ReadProcessMemory(hProcess, reinterpret_cast<const void*>(moduleBase + dos.e_lfanew),
                           &nt, sizeof(nt), nullptr) || nt.Signature != IMAGE_NT_SIGNATURE ||
        nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC || nt.OptionalHeader.SizeOfImage == 0)
        return 0;

    const DWORD imageSize = nt.OptionalHeader.SizeOfImage;
    size_t patched = 0;
    const IMAGE_DATA_DIRECTORY imports = nt.OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_IMPORT
        ? nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]
        : IMAGE_DATA_DIRECTORY {};
    if (imports.VirtualAddress && imports.VirtualAddress < imageSize) {
        const size_t descriptorLimit = imports.Size
            ? (std::min)(static_cast<size_t>(imports.Size / sizeof(IMAGE_IMPORT_DESCRIPTOR) + 1), static_cast<size_t>(4096))
            : 4096;
        for (size_t index = 0; index < descriptorLimit; ++index) {
            IMAGE_IMPORT_DESCRIPTOR descriptor = {};
            if (!ReadProcessMemory(hProcess,
                                   reinterpret_cast<const void*>(moduleBase + imports.VirtualAddress + index * sizeof(descriptor)),
                                   &descriptor, sizeof(descriptor), nullptr))
                break;
            if (descriptor.Name == 0 && descriptor.FirstThunk == 0)
                break;
            if (!descriptor.Name || descriptor.Name >= imageSize)
                continue;

            char importedModule[128] = {};
            if (!ReadRemoteAscii(hProcess, moduleBase + descriptor.Name, importedModule, sizeof(importedModule)))
                continue;
            size_t tablePatched = PatchRemoteThunkTable(
                hProcess, moduleBase, imageSize, descriptor.OriginalFirstThunk, descriptor.FirstThunk,
                importedModule, targetModule, targetFunc, targetAddress, detourAddress, moduleName);
            if (tablePatched == (std::numeric_limits<size_t>::max)())
                return tablePatched;
            patched += tablePatched;
        }
    }

    // Also cover delay imports that have not resolved yet.  Replacing the delay-IAT slot directly
    // intentionally bypasses the delay helper; the MinHook trampoline already reaches the target.
    const IMAGE_DATA_DIRECTORY delays = nt.OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT
        ? nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT]
        : IMAGE_DATA_DIRECTORY {};
    if (delays.VirtualAddress && delays.VirtualAddress < imageSize) {
        const size_t descriptorLimit = delays.Size
            ? (std::min)(static_cast<size_t>(delays.Size / sizeof(DelayImportDescriptor) + 1), static_cast<size_t>(4096))
            : 4096;
        for (size_t index = 0; index < descriptorLimit; ++index) {
            DelayImportDescriptor descriptor = {};
            if (!ReadProcessMemory(hProcess,
                                   reinterpret_cast<const void*>(moduleBase + delays.VirtualAddress + index * sizeof(descriptor)),
                                   &descriptor, sizeof(descriptor), nullptr))
                break;
            if (descriptor.name == 0 && descriptor.iat == 0)
                break;
            if ((descriptor.attributes & 1) == 0 || !descriptor.name || descriptor.name >= imageSize)
                continue; // PE32+ delay descriptors emitted by modern linkers use RVA fields.

            char importedModule[128] = {};
            if (!ReadRemoteAscii(hProcess, moduleBase + descriptor.name, importedModule, sizeof(importedModule)))
                continue;
            size_t tablePatched = PatchRemoteThunkTable(
                hProcess, moduleBase, imageSize, descriptor.intTable, descriptor.iat,
                importedModule, targetModule, targetFunc, targetAddress, detourAddress, moduleName);
            if (tablePatched == (std::numeric_limits<size_t>::max)())
                return tablePatched;
            patched += tablePatched;
        }
    }
    return patched;
}

bool InstallEarlySocketRedirect(HANDLE hProcess, uintptr_t playerModuleBase,
                                uintptr_t bootstrapBase,
                                const std::vector<uint8_t>& bootstrapData,
                                uint16_t emulatorHttpPort, uint16_t emulatorHttpsPort) {
    const uintptr_t getProcAddressRva =
        GetImageExportRva(bootstrapData, "NoobEarlyGetProcAddress");
    const uintptr_t originalGetProcAddressRva =
        GetImageExportRva(bootstrapData, "NoobEarlyOriginalGetProcAddress");
    const uintptr_t httpPortRva = GetImageExportRva(bootstrapData, "NoobEarlyHttpPort");
    const uintptr_t httpsPortRva = GetImageExportRva(bootstrapData, "NoobEarlyHttpsPort");
    if (!getProcAddressRva || !originalGetProcAddressRva || !httpPortRva || !httpsPortRva) {
        printf("EarlyRedirect: required bootstrap exports are missing\n");
        return false;
    }

    HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
    FARPROC localGetProcAddress = kernel32 ? GetProcAddress(kernel32, "GetProcAddress") : nullptr;
    HMODULE localOwner = nullptr;
    if (!localGetProcAddress ||
        !GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCSTR>(localGetProcAddress), &localOwner)) {
        printf("EarlyRedirect: cannot resolve the local GetProcAddress owner\n");
        return false;
    }
    wchar_t ownerName[MAX_PATH] = {};
    if (!GetModuleBaseNameW(GetCurrentProcess(), localOwner, ownerName, _countof(ownerName))) {
        printf("EarlyRedirect: cannot identify the GetProcAddress owner module\n");
        return false;
    }
    const uintptr_t remoteOwner = GetRemoteModuleBase(hProcess, ownerName);
    if (!remoteOwner) {
        printf("EarlyRedirect: remote GetProcAddress owner %ls is not loaded\n", ownerName);
        return false;
    }
    const uintptr_t remoteGetProcAddress = remoteOwner +
        (reinterpret_cast<uintptr_t>(localGetProcAddress) - reinterpret_cast<uintptr_t>(localOwner));

    auto writeBootstrapValue = [&](uintptr_t rva, const void* value, SIZE_T size,
                                   const char* label) -> bool {
        SIZE_T written = 0;
        const NTSTATUS status = SysNtWriteVirtualMemory(
            hProcess, reinterpret_cast<void*>(bootstrapBase + rva),
            const_cast<void*>(value), size, &written);
        if (status < 0 || written != size) {
            printf("EarlyRedirect: cannot initialize %s (status=0x%08lX, wrote=%zu)\n",
                   label, static_cast<unsigned long>(status), written);
            return false;
        }
        return true;
    };

    if (!writeBootstrapValue(originalGetProcAddressRva, &remoteGetProcAddress,
                             sizeof(remoteGetProcAddress), "original GetProcAddress") ||
        !writeBootstrapValue(httpPortRva, &emulatorHttpPort,
                             sizeof(emulatorHttpPort), "HTTP port") ||
        !writeBootstrapValue(httpsPortRva, &emulatorHttpsPort,
                             sizeof(emulatorHttpsPort), "HTTPS port"))
        return false;

    const uintptr_t detourAddress = bootstrapBase + getProcAddressRva;
    size_t patched = PatchModuleIat(
        hProcess, reinterpret_cast<HMODULE>(playerModuleBase), L"RobloxPlayerBeta.dll",
        "kernel32.dll", "GetProcAddress", remoteGetProcAddress, detourAddress);
    if (patched == (std::numeric_limits<size_t>::max)()) {
        printf("EarlyRedirect: Player GetProcAddress IAT write became inconsistent\n");
        return false;
    }
    if (patched == 0) {
        printf("EarlyRedirect: Player GetProcAddress IAT slot was not patched\n");
        return false;
    }

    printf("EarlyRedirect: installed image-backed GetProcAddress bridge at 0x%llX "
           "for HTTP=%u HTTPS=%u (%zu slot)\n",
           (unsigned long long)detourAddress, emulatorHttpPort, emulatorHttpsPort, patched);
    return true;
}

void LogEarlySocketRedirectStats(HANDLE hProcess, uintptr_t bootstrapBase,
                                 const std::vector<uint8_t>& bootstrapData) {
    struct Counter {
        const char* exportName;
        uint64_t value;
    } counters[] = {
        {"NoobEarlyGetProcAddressCalls", 0},
        {"NoobEarlyConnectResolutions", 0},
        {"NoobEarlyWsaConnectResolutions", 0},
        {"NoobEarlySocketCalls", 0},
        {"NoobEarlyWspConnectCalls", 0},
        {"NoobEarlyRedirectedSocketCalls", 0},
    };

    bool complete = bootstrapBase != 0;
    for (Counter& counter : counters) {
        const uintptr_t rva = GetImageExportRva(bootstrapData, counter.exportName);
        SIZE_T read = 0;
        if (!rva || !ReadProcessMemory(hProcess,
                                       reinterpret_cast<const void*>(bootstrapBase + rva),
                                       &counter.value, sizeof(counter.value), &read) ||
            read != sizeof(counter.value)) {
            complete = false;
        }
    }
    if (!complete) {
        printf("EarlyRedirect: counters unavailable\n");
        return;
    }
    printf("EarlyRedirect: stats GetProcAddress=%llu connect-resolve=%llu "
            "WSAConnect-resolve=%llu socket-calls=%llu WSPConnect=%llu redirected=%llu\n",
            (unsigned long long)counters[0].value,
           (unsigned long long)counters[1].value,
           (unsigned long long)counters[2].value,
           (unsigned long long)counters[3].value,
            (unsigned long long)counters[4].value,
            (unsigned long long)counters[5].value);
}

// libcurl resolves Winsock entry points dynamically and caches the returned
// addresses in image data.  Such slots are not described by the PE import directory, so cover them
// by replacing exact export-address matches in non-executable data sections.
static size_t PatchModuleResolvedPointers(HANDLE hProcess, HMODULE module,
                                          const wchar_t* moduleName,
                                          const char* targetFunc,
                                          uintptr_t targetAddress,
                                          uintptr_t detourAddress) {
    const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(module);
    IMAGE_DOS_HEADER dos = {};
    if (!ReadProcessMemory(hProcess, module, &dos, sizeof(dos), nullptr) ||
        dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew <= 0 || dos.e_lfanew > 0x100000)
        return 0;

    IMAGE_NT_HEADERS64 nt = {};
    if (!ReadProcessMemory(hProcess, reinterpret_cast<const void*>(moduleBase + dos.e_lfanew),
                           &nt, sizeof(nt), nullptr) || nt.Signature != IMAGE_NT_SIGNATURE ||
        nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC || nt.OptionalHeader.SizeOfImage == 0)
        return 0;

    std::vector<IMAGE_SECTION_HEADER> sections(nt.FileHeader.NumberOfSections);
    const uintptr_t sectionTable = moduleBase + dos.e_lfanew +
        offsetof(IMAGE_NT_HEADERS64, OptionalHeader) + nt.FileHeader.SizeOfOptionalHeader;
    SIZE_T sectionBytes = sections.size() * sizeof(IMAGE_SECTION_HEADER);
    SIZE_T sectionsRead = 0;
    if (sections.empty() ||
        !ReadProcessMemory(hProcess, reinterpret_cast<const void*>(sectionTable), sections.data(),
                           sectionBytes, &sectionsRead) || sectionsRead != sectionBytes)
        return 0;

    size_t patched = 0;
    const bool playerSocketTable =
        _wcsicmp(moduleName, L"RobloxPlayerBeta.exe") == 0 &&
        (strcmp(targetFunc, "connect") == 0 || strcmp(targetFunc, "WSAConnect") == 0);
    for (const IMAGE_SECTION_HEADER& section : sections) {
        const DWORD characteristics = section.Characteristics;
        if (!(characteristics & IMAGE_SCN_MEM_READ) ||
            (!(characteristics & IMAGE_SCN_MEM_WRITE) && !playerSocketTable) ||
            (characteristics & IMAGE_SCN_MEM_EXECUTE) ||
            section.VirtualAddress >= nt.OptionalHeader.SizeOfImage)
            continue;

        const size_t virtualSize = (std::min)(
            static_cast<size_t>(section.Misc.VirtualSize),
            static_cast<size_t>(nt.OptionalHeader.SizeOfImage - section.VirtualAddress));
        uintptr_t cursor = moduleBase + section.VirtualAddress;
        const uintptr_t sectionEnd = cursor + virtualSize;
        while (cursor + sizeof(uintptr_t) <= sectionEnd) {
            MEMORY_BASIC_INFORMATION memory = {};
            if (VirtualQueryEx(hProcess, reinterpret_cast<const void*>(cursor),
                               &memory, sizeof(memory)) != sizeof(memory))
                break;

            const uintptr_t regionEnd = (std::min)(
                sectionEnd,
                reinterpret_cast<uintptr_t>(memory.BaseAddress) + memory.RegionSize);
            if (regionEnd <= cursor)
                break;
            const bool runtimeWritable = IsWritableProtection(memory.Protect);
            if (memory.State != MEM_COMMIT || (!runtimeWritable && !playerSocketTable) ||
                IsExecutableProtection(memory.Protect) ||
                (memory.Protect & (PAGE_GUARD | PAGE_NOACCESS))) {
                cursor = regionEnd;
                continue;
            }

            constexpr size_t kReadChunk = 64 * 1024;
            while (cursor + sizeof(uintptr_t) <= regionEnd) {
                const size_t bytesToRead = (std::min)(
                    kReadChunk, static_cast<size_t>(regionEnd - cursor));
                std::vector<uint8_t> bytes(bytesToRead);
                SIZE_T bytesRead = 0;
                if (!ReadProcessMemory(hProcess, reinterpret_cast<const void*>(cursor), bytes.data(),
                                       bytes.size(), &bytesRead) || bytesRead < sizeof(uintptr_t)) {
                    cursor += bytesToRead;
                    continue;
                }

                const uintptr_t firstAligned = (cursor + sizeof(uintptr_t) - 1) &
                                               ~(static_cast<uintptr_t>(sizeof(uintptr_t) - 1));
                for (uintptr_t slot = firstAligned;
                     slot + sizeof(uintptr_t) <= cursor + bytesRead;
                     slot += sizeof(uintptr_t)) {
                    uintptr_t value = 0;
                    memcpy(&value, bytes.data() + (slot - cursor), sizeof(value));
                    if (value != targetAddress)
                        continue;

                    IatWriteResult result = WriteRemoteIatPointer(hProcess, slot, detourAddress);
                    if (result == IatWriteResult::Installed) {
                        printf("InstallHooks: resolved-pointer fallback %ls slot 0x%llX -> 0x%llX OK\n",
                               moduleName, (unsigned long long)slot,
                               (unsigned long long)detourAddress);
                        ++patched;
                    } else if (result == IatWriteResult::Inconsistent) {
                        printf("InstallHooks: resolved-pointer fallback %ls slot 0x%llX could not be rolled back\n",
                               moduleName, (unsigned long long)slot);
                        return (std::numeric_limits<size_t>::max)();
                    }
                }
                cursor += bytesToRead;
            }
        }
    }
    return patched;
}

static size_t InstallIatFallback(HANDLE hProcess, uintptr_t hookDllBase,
                                 const char* targetModule, const char* targetFunc,
                                 uintptr_t targetAddress, uintptr_t detourAddress) {
    DWORD bytesNeeded = 0;
    std::vector<HMODULE> modules(256);
    for (;;) {
        if (!EnumProcessModulesEx(hProcess, modules.data(),
                                  static_cast<DWORD>(modules.size() * sizeof(HMODULE)),
                                  &bytesNeeded, LIST_MODULES_ALL))
            return 0;
        if (bytesNeeded <= modules.size() * sizeof(HMODULE))
            break;
        modules.resize(bytesNeeded / sizeof(HMODULE) + 32);
    }

    size_t patched = 0;
    const size_t moduleCount = bytesNeeded / sizeof(HMODULE);
    for (size_t index = 0; index < moduleCount; ++index) {
        if (reinterpret_cast<uintptr_t>(modules[index]) == hookDllBase)
            continue;
        wchar_t moduleName[MAX_PATH] = {};
        if (IsSystemModule(hProcess, modules[index], moduleName, _countof(moduleName)))
            continue;
        if (_wcsnicmp(moduleName, L"noobhook", 8) == 0)
            continue; // Its own imports must keep reaching the original trampoline.
        size_t modulePatched = PatchModuleIat(hProcess, modules[index], moduleName, targetModule, targetFunc,
                                              targetAddress, detourAddress);
        if (modulePatched == (std::numeric_limits<size_t>::max)())
            return modulePatched;
        patched += modulePatched;

        modulePatched = PatchModuleResolvedPointers(hProcess, modules[index], moduleName,
                                                    targetFunc, targetAddress, detourAddress);
        if (modulePatched == (std::numeric_limits<size_t>::max)())
            return modulePatched;
        patched += modulePatched;
    }
    return patched;
}

// The 0.728 app shell also keeps runtime-resolved Winsock pointers in private heap-backed
// dispatch objects.  Those addresses are outside every PE section and therefore invisible to the
// import-table and image-data passes above.  Scan only committed, non-executable private writable
// memory and replace exact export-address matches.  Restricting this fallback to socket entry
// points avoids rewriting unrelated cached process APIs.
static size_t PatchRemotePrivateSocketPointers(HANDLE hProcess, const char* targetFunc,
                                               uintptr_t targetAddress,
                                               uintptr_t detourAddress) {
    if (strcmp(targetFunc, "connect") != 0 && strcmp(targetFunc, "WSAConnect") != 0)
        return 0;

    SYSTEM_INFO systemInfo = {};
    GetSystemInfo(&systemInfo);
    uintptr_t cursor = reinterpret_cast<uintptr_t>(systemInfo.lpMinimumApplicationAddress);
    const uintptr_t maximum = reinterpret_cast<uintptr_t>(systemInfo.lpMaximumApplicationAddress);
    size_t patched = 0;
    size_t scannedBytes = 0;
    constexpr size_t kReadChunk = 256 * 1024;
    constexpr size_t kMaximumPatches = 256;

    while (cursor < maximum) {
        MEMORY_BASIC_INFORMATION memory = {};
        if (VirtualQueryEx(hProcess, reinterpret_cast<const void*>(cursor),
                           &memory, sizeof(memory)) != sizeof(memory))
            break;
        const uintptr_t regionBase = reinterpret_cast<uintptr_t>(memory.BaseAddress);
        const uintptr_t regionEnd = regionBase + memory.RegionSize;
        if (regionEnd <= cursor)
            break;

        const bool scanRegion = memory.State == MEM_COMMIT && memory.Type == MEM_PRIVATE &&
            IsWritableProtection(memory.Protect) && !IsExecutableProtection(memory.Protect) &&
            !(memory.Protect & (PAGE_GUARD | PAGE_NOACCESS));
        if (!scanRegion) {
            cursor = regionEnd;
            continue;
        }

        uintptr_t chunkBase = regionBase;
        while (chunkBase + sizeof(uintptr_t) <= regionEnd) {
            const size_t chunkSize = (std::min)(
                kReadChunk, static_cast<size_t>(regionEnd - chunkBase));
            std::vector<uint8_t> bytes(chunkSize);
            SIZE_T bytesRead = 0;
            ReadProcessMemory(hProcess, reinterpret_cast<const void*>(chunkBase),
                              bytes.data(), bytes.size(), &bytesRead);
            scannedBytes += bytesRead;
            if (bytesRead >= sizeof(uintptr_t)) {
                const uintptr_t firstAligned = (chunkBase + sizeof(uintptr_t) - 1) &
                    ~(static_cast<uintptr_t>(sizeof(uintptr_t) - 1));
                for (uintptr_t slot = firstAligned;
                     slot + sizeof(uintptr_t) <= chunkBase + bytesRead;
                     slot += sizeof(uintptr_t)) {
                    uintptr_t value = 0;
                    memcpy(&value, bytes.data() + (slot - chunkBase), sizeof(value));
                    if (value != targetAddress)
                        continue;

                    IatWriteResult result = WriteRemoteIatPointer(hProcess, slot, detourAddress);
                    if (result == IatWriteResult::Installed) {
                        printf("InstallHooks: private-pointer fallback %s slot 0x%llX -> 0x%llX OK\n",
                               targetFunc, (unsigned long long)slot,
                               (unsigned long long)detourAddress);
                        if (++patched >= kMaximumPatches) {
                            printf("InstallHooks: private-pointer fallback %s reached safety limit\n",
                                   targetFunc);
                            return patched;
                        }
                    } else if (result == IatWriteResult::Inconsistent) {
                        printf("InstallHooks: private-pointer fallback %s slot 0x%llX could not be rolled back\n",
                               targetFunc, (unsigned long long)slot);
                        return (std::numeric_limits<size_t>::max)();
                    }
                }
            }
            chunkBase += chunkSize;
        }
        cursor = regionEnd;
    }

    printf("InstallHooks: private-pointer fallback %s scanned %zu MiB and replaced %zu slot(s)\n",
           targetFunc, scannedBytes / (1024 * 1024), patched);
    return patched;
}

// The Winsock SPI table has 30 pointer-sized entries; WSPConnect is entry seven. WS2_32 keeps a
// heap-backed copy and dispatches connect through that copy. Changing only this writable provider
// table avoids copy-on-write changes to both the Player and Windows image pages.
constexpr size_t kWspProcCount = 30;
constexpr size_t kWspConnectIndex = 7;

static uintptr_t FindLocalWspProcTable(HMODULE module) {
    if (!module)
        return 0;
    const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(module);
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(moduleBase);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0)
        return 0;
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(moduleBase + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        return 0;

    const auto* startup = reinterpret_cast<const uint8_t*>(GetProcAddress(module, "WSPStartup"));
    if (!startup)
        return 0;
    constexpr uint8_t kCopySequence[] = {
        0x0F, 0x10, 0x00,             // movups xmm0,[rax]
        0x0F, 0x10, 0x48, 0x10,       // movups xmm1,[rax+10h]
        0x0F, 0x11, 0x01              // movups [rcx],xmm0
    };
    for (size_t offset = 0; offset + 7 + sizeof(kCopySequence) <= 0x300; ++offset) {
        if (startup[offset] != 0x48 || startup[offset + 1] != 0x8D ||
            startup[offset + 2] != 0x05 ||
            memcmp(startup + offset + 7, kCopySequence, sizeof(kCopySequence)) != 0)
            continue;
        int32_t displacement = 0;
        memcpy(&displacement, startup + offset + 3, sizeof(displacement));
        const uintptr_t table = reinterpret_cast<uintptr_t>(startup + offset + 7) +
                                displacement;
        const uintptr_t imageEnd = moduleBase + nt->OptionalHeader.SizeOfImage;
        if (table >= moduleBase && table + kWspProcCount * sizeof(uintptr_t) <= imageEnd)
            return table;
    }
    return 0;
}

static bool MarkRemoteCfgTarget(HANDLE hProcess, uintptr_t target) {
    using SetProcessValidCallTargetsFn = BOOL(WINAPI*)(
        HANDLE, PVOID, SIZE_T, ULONG, PCFG_CALL_TARGET_INFO);
    FARPROC setTargetProc = GetProcAddress(
        GetModuleHandleW(L"kernel32.dll"), "SetProcessValidCallTargets");
    if (!setTargetProc)
        setTargetProc = GetProcAddress(
            GetModuleHandleW(L"kernelbase.dll"), "SetProcessValidCallTargets");
    const auto setProcessValidCallTargets =
        reinterpret_cast<SetProcessValidCallTargetsFn>(setTargetProc);
    if (!setProcessValidCallTargets) {
        printf("InstallHooks: SetProcessValidCallTargets is unavailable\n");
        return false;
    }
    SYSTEM_INFO systemInfo = {};
    GetSystemInfo(&systemInfo);
    const uintptr_t pageMask = static_cast<uintptr_t>(systemInfo.dwPageSize - 1);
    const uintptr_t page = target & ~pageMask;
    CFG_CALL_TARGET_INFO targetInfo = {};
    targetInfo.Offset = target - page;
    targetInfo.Flags = CFG_CALL_TARGET_VALID;
    if (setProcessValidCallTargets(hProcess, reinterpret_cast<void*>(page),
                                   systemInfo.dwPageSize, 1, &targetInfo)) {
        printf("InstallHooks: registered WSPConnect detour 0x%llX with CFG\n",
               static_cast<unsigned long long>(target));
        return true;
    }
    // SEC_IMAGE pages normally begin with valid-call-target bits enabled. Some Windows builds
    // therefore reject the redundant update; log it, but let the dispatch table test that state.
    printf("InstallHooks: CFG registration for WSPConnect detour returned error %lu\n",
           GetLastError());
    return false;
}

static bool PublishRemoteOriginal(HANDLE hProcess, uintptr_t hookDllBase,
                                  size_t hookDllSize, void** remoteStorage,
                                  uintptr_t original) {
    const uintptr_t storage = reinterpret_cast<uintptr_t>(remoteStorage);
    if (!storage || storage < hookDllBase || storage + sizeof(original) > hookDllBase + hookDllSize) {
        printf("InstallHooks: WSPConnect original storage 0x%llX is outside noobHook\n",
               static_cast<unsigned long long>(storage));
        return false;
    }
    SIZE_T written = 0;
    const NTSTATUS status = SysNtWriteVirtualMemory(
        hProcess, reinterpret_cast<void*>(storage), &original, sizeof(original), &written);
    uintptr_t observed = 0;
    SIZE_T read = 0;
    const bool published = status >= 0 && written == sizeof(original) &&
        ReadProcessMemory(hProcess, reinterpret_cast<const void*>(storage),
                          &observed, sizeof(observed), &read) &&
        read == sizeof(observed) && observed == original;
    printf("InstallHooks: WSPConnect original 0x%llX -> storage 0x%llX %s\n",
           static_cast<unsigned long long>(original),
           static_cast<unsigned long long>(storage), published ? "OK" : "FAILED");
    return published;
}

static size_t PatchWinsockProviderTables(HANDLE hProcess, uintptr_t detourAddress,
                                            void** remoteOriginalStorage,
                                            uintptr_t hookDllBase, size_t hookDllSize) {
    HMODULE localMswsock = GetModuleHandleW(L"mswsock.dll");
    if (!localMswsock)
        localMswsock = LoadLibraryW(L"mswsock.dll");
    const uintptr_t localTable = FindLocalWspProcTable(localMswsock);
    if (!localTable) {
        printf("InstallHooks: cannot locate the local MSWSOCK WSPPROC_TABLE template\n");
        return 0;
    }

    const uintptr_t remoteMswsock = GetRemoteModuleBase(hProcess, L"mswsock.dll");
    if (!remoteMswsock) {
        printf("InstallHooks: MSWSOCK is not loaded in the Player\n");
        return 0;
    }
    const uintptr_t remoteTable = remoteMswsock +
        (localTable - reinterpret_cast<uintptr_t>(localMswsock));
    std::array<uintptr_t, kWspProcCount> expected = {};
    SIZE_T read = 0;
    if (!ReadProcessMemory(hProcess, reinterpret_cast<const void*>(remoteTable),
                           expected.data(), sizeof(expected), &read) || read != sizeof(expected) ||
        !expected[kWspConnectIndex]) {
        printf("InstallHooks: cannot read the remote MSWSOCK WSPPROC_TABLE template\n");
        return 0;
    }
    const uintptr_t original = expected[kWspConnectIndex];
    printf("InstallHooks: MSWSOCK WSPPROC_TABLE template 0x%llX, WSPConnect=0x%llX\n",
           static_cast<unsigned long long>(remoteTable),
           static_cast<unsigned long long>(original));
    if (!PublishRemoteOriginal(hProcess, hookDllBase, hookDllSize,
                               remoteOriginalStorage, original))
        return 0;
    MarkRemoteCfgTarget(hProcess, detourAddress);

    constexpr size_t kChunkSize = 512 * 1024;
    constexpr size_t kMaximumPatches = 32;
    SYSTEM_INFO systemInfo = {};
    GetSystemInfo(&systemInfo);
    uintptr_t cursor = reinterpret_cast<uintptr_t>(systemInfo.lpMinimumApplicationAddress);
    const uintptr_t maximum = reinterpret_cast<uintptr_t>(systemInfo.lpMaximumApplicationAddress);
    std::vector<uintptr_t> patchedSlots;
    size_t scannedBytes = 0;

    while (cursor < maximum && patchedSlots.size() < kMaximumPatches) {
        MEMORY_BASIC_INFORMATION memory = {};
        if (VirtualQueryEx(hProcess, reinterpret_cast<const void*>(cursor),
                           &memory, sizeof(memory)) != sizeof(memory))
            break;
        const uintptr_t regionBase = reinterpret_cast<uintptr_t>(memory.BaseAddress);
        const uintptr_t regionEnd = regionBase + memory.RegionSize;
        if (regionEnd <= cursor)
            break;
        const bool scanRegion = memory.State == MEM_COMMIT && memory.Type == MEM_PRIVATE &&
            IsWritableProtection(memory.Protect) && !IsExecutableProtection(memory.Protect) &&
            !(memory.Protect & (PAGE_GUARD | PAGE_NOACCESS));
        if (!scanRegion) {
            cursor = regionEnd;
            continue;
        }

        uintptr_t chunkBase = regionBase;
        while (chunkBase + sizeof(uintptr_t) <= regionEnd &&
               patchedSlots.size() < kMaximumPatches) {
            const size_t chunkSize = (std::min)(
                kChunkSize, static_cast<size_t>(regionEnd - chunkBase));
            std::vector<uint8_t> bytes(chunkSize);
            SIZE_T bytesRead = 0;
            ReadProcessMemory(hProcess, reinterpret_cast<const void*>(chunkBase),
                              bytes.data(), bytes.size(), &bytesRead);
            scannedBytes += bytesRead;
            if (bytesRead >= sizeof(uintptr_t)) {
                const uintptr_t alignedStart = (chunkBase + sizeof(uintptr_t) - 1) &
                                               ~(static_cast<uintptr_t>(sizeof(uintptr_t) - 1));
                for (uintptr_t slot = alignedStart;
                     slot + sizeof(uintptr_t) <= chunkBase + bytesRead;
                     slot += sizeof(uintptr_t)) {
                    uintptr_t value = 0;
                    memcpy(&value, bytes.data() + (slot - chunkBase), sizeof(value));
                    if (value != original)
                        continue;
                    if (std::find(patchedSlots.begin(), patchedSlots.end(), slot) !=
                        patchedSlots.end())
                        continue;

                    // WSPStartup may specialize some neighboring entries before WS2_32 stores the
                    // table, so an all-entry template comparison is not stable across Windows
                    // builds. The WSPConnect implementation pointer itself is internal to
                    // MSWSOCK (not an exported general-purpose API), making an exact occurrence in
                    // writable private memory the durable provider-table identifier. Count exact
                    // neighbors for diagnostics without rejecting a specialized table.
                    const uintptr_t tableStart = slot >= kWspConnectIndex * sizeof(uintptr_t)
                        ? slot - kWspConnectIndex * sizeof(uintptr_t) : 0;
                    std::array<uintptr_t, kWspProcCount> candidate = {};
                    SIZE_T candidateRead = 0;
                    size_t matchingEntries = 0;
                    if (tableStart && ReadProcessMemory(
                            hProcess, reinterpret_cast<const void*>(tableStart),
                            candidate.data(), sizeof(candidate), &candidateRead) &&
                        candidateRead == sizeof(candidate)) {
                        for (size_t entry = 0; entry < kWspProcCount; ++entry) {
                            if (candidate[entry] == expected[entry])
                                ++matchingEntries;
                        }
                    }
                    printf("InstallHooks: private WSPConnect slot candidate 0x%llX "
                           "(template neighbors %zu/%zu)\n",
                           static_cast<unsigned long long>(slot), matchingEntries,
                           kWspProcCount);
                    const IatWriteResult result = WriteRemoteIatPointer(
                        hProcess, slot, detourAddress);
                    if (result == IatWriteResult::Installed) {
                        patchedSlots.push_back(slot);
                        printf("InstallHooks: WSPPROC_TABLE 0x%llX connect slot 0x%llX -> "
                               "0x%llX OK\n",
                               static_cast<unsigned long long>(tableStart),
                               static_cast<unsigned long long>(slot),
                               static_cast<unsigned long long>(detourAddress));
                    } else if (result == IatWriteResult::Inconsistent) {
                        printf("InstallHooks: WSPPROC_TABLE slot could not be rolled back\n");
                        return (std::numeric_limits<size_t>::max)();
                    }
                }
            }
            if (chunkBase + chunkSize >= regionEnd)
                break;
            chunkBase += chunkSize;
        }
        cursor = regionEnd;
    }

    printf("InstallHooks: WSPPROC_TABLE scan covered %zu MiB and patched %zu table(s)\n",
           scannedBytes / (1024 * 1024), patchedSlots.size());
    return patchedSlots.size();
}

size_t InstallEarlyWspRedirect(HANDLE hProcess, uintptr_t bootstrapBase,
                               const std::vector<uint8_t>& bootstrapData,
                               uint16_t emulatorHttpPort, uint16_t emulatorHttpsPort) {
    const uintptr_t detourRva =
        GetImageExportRva(bootstrapData, "NoobEarlyWspConnect");
    const uintptr_t originalRva =
        GetImageExportRva(bootstrapData, "NoobEarlyOriginalWspConnect");
    const uintptr_t httpPortRva =
        GetImageExportRva(bootstrapData, "NoobEarlyHttpPort");
    const uintptr_t httpsPortRva =
        GetImageExportRva(bootstrapData, "NoobEarlyHttpsPort");
    if (!bootstrapBase || !detourRva || !originalRva || !httpPortRva || !httpsPortRva) {
        printf("EarlyRedirect: WSP bootstrap exports are missing\n");
        return 0;
    }

    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(bootstrapData.data());
    if (bootstrapData.size() < sizeof(*dos) || dos->e_magic != IMAGE_DOS_SIGNATURE ||
        dos->e_lfanew <= 0 ||
        static_cast<size_t>(dos->e_lfanew) + sizeof(IMAGE_NT_HEADERS64) > bootstrapData.size()) {
        printf("EarlyRedirect: WSP bootstrap PE headers are invalid\n");
        return 0;
    }
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(
        bootstrapData.data() + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        printf("EarlyRedirect: WSP bootstrap is not PE32+\n");
        return 0;
    }

    auto writeValue = [&](uintptr_t address, const void* value, SIZE_T size,
                          const char* label) -> bool {
        SIZE_T written = 0;
        const NTSTATUS status = SysNtWriteVirtualMemory(
            hProcess, reinterpret_cast<void*>(address), const_cast<void*>(value), size, &written);
        if (status >= 0 && written == size)
            return true;
        printf("EarlyRedirect: cannot initialize %s (status=0x%08lX, wrote=%zu)\n",
               label, static_cast<unsigned long>(status), written);
        return false;
    };
    if (!writeValue(bootstrapBase + httpPortRva, &emulatorHttpPort,
                    sizeof(emulatorHttpPort), "HTTP port") ||
        !writeValue(bootstrapBase + httpsPortRva, &emulatorHttpsPort,
                    sizeof(emulatorHttpsPort), "HTTPS port"))
        return 0;

    return PatchWinsockProviderTables(
        hProcess, bootstrapBase + detourRva,
        reinterpret_cast<void**>(bootstrapBase + originalRva),
        bootstrapBase, nt->OptionalHeader.SizeOfImage);
}

LegacyBytecodeFlagState ReassertLegacyBytecodeFlags(HANDLE hProcess,
                                                    uintptr_t playerBase,
                                                    RobloxEra era) {
    if (era != RobloxEra::Era719 || playerBase == 0)
        return LegacyBytecodeFlagState::Unsupported;

    constexpr DWORD kTimeDateStamp = 0xE75D9ACB;
    constexpr DWORD kSizeOfImage = 0x081D8000;
    constexpr uintptr_t kOptimizeMinRva = 0x074E2A58;
    constexpr uintptr_t kOptimizeMinNameRva = 0x0633F320;
    constexpr uintptr_t kLoadRawRecordRva = 0x07631308;
    constexpr uintptr_t kLoadRawNameRva = 0x064619B0;
    constexpr char kOptimizeMinName[] = "LsbOptimizeMin";
    constexpr char kLoadRawName[] = "LoadRawBytecodeWithHashKey";

    IMAGE_DOS_HEADER dos = {};
    IMAGE_NT_HEADERS64 nt = {};
    if (!ReadProcessMemory(hProcess, reinterpret_cast<const void*>(playerBase),
                           &dos, sizeof(dos), nullptr) ||
        dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew <= 0 ||
        dos.e_lfanew > 0x100000 ||
        !ReadProcessMemory(hProcess,
                           reinterpret_cast<const void*>(playerBase + dos.e_lfanew),
                           &nt, sizeof(nt), nullptr) ||
        nt.Signature != IMAGE_NT_SIGNATURE ||
        nt.FileHeader.TimeDateStamp != kTimeDateStamp ||
        nt.OptionalHeader.SizeOfImage != kSizeOfImage) {
        return LegacyBytecodeFlagState::Unsupported;
    }

    char optimizeName[sizeof(kOptimizeMinName)] = {};
    char loadRawName[sizeof(kLoadRawName)] = {};
    if (!ReadProcessMemory(hProcess,
                           reinterpret_cast<const void*>(playerBase + kOptimizeMinNameRva),
                           optimizeName, sizeof(optimizeName), nullptr) ||
        memcmp(optimizeName, kOptimizeMinName, sizeof(optimizeName)) != 0 ||
        !ReadProcessMemory(hProcess,
                           reinterpret_cast<const void*>(playerBase + kLoadRawNameRva),
                           loadRawName, sizeof(loadRawName), nullptr) ||
        memcmp(loadRawName, kLoadRawName, sizeof(loadRawName)) != 0) {
        return LegacyBytecodeFlagState::WaitingForPlayerInitialization;
    }

    struct BoolFastVariableRecord {
        uint8_t value;
        uint8_t padding[7];
        uintptr_t name;
        size_t nameLength;
    };
    static_assert(sizeof(BoolFastVariableRecord) == 24);

    int32_t optimizeMin = -1;
    BoolFastVariableRecord loadRaw = {};
    if (!ReadProcessMemory(hProcess,
                           reinterpret_cast<const void*>(playerBase + kOptimizeMinRva),
                           &optimizeMin, sizeof(optimizeMin), nullptr) ||
        !ReadProcessMemory(hProcess,
                           reinterpret_cast<const void*>(playerBase + kLoadRawRecordRva),
                           &loadRaw, sizeof(loadRaw), nullptr) ||
        (optimizeMin != 0 && optimizeMin != 1) ||
        loadRaw.value > 1 ||
        loadRaw.name != playerBase + kLoadRawNameRva ||
        loadRaw.nameLength != sizeof(kLoadRawName) - 1) {
        return LegacyBytecodeFlagState::WaitingForPlayerInitialization;
    }

    const int32_t zeroInt = 0;
    const uint8_t zeroBool = 0;
    SIZE_T written = 0;
    NTSTATUS status = SysNtWriteVirtualMemory(
        hProcess, reinterpret_cast<void*>(playerBase + kOptimizeMinRva),
        const_cast<int32_t*>(&zeroInt), sizeof(zeroInt), &written);
    if (status < 0 || written != sizeof(zeroInt))
        return LegacyBytecodeFlagState::WriteFailed;

    written = 0;
    status = SysNtWriteVirtualMemory(
        hProcess, reinterpret_cast<void*>(playerBase + kLoadRawRecordRva),
        const_cast<uint8_t*>(&zeroBool), sizeof(zeroBool), &written);
    if (status < 0 || written != sizeof(zeroBool))
        return LegacyBytecodeFlagState::WriteFailed;

    optimizeMin = -1;
    loadRaw.value = 0xff;
    if (!ReadProcessMemory(hProcess,
                           reinterpret_cast<const void*>(playerBase + kOptimizeMinRva),
                           &optimizeMin, sizeof(optimizeMin), nullptr) ||
        !ReadProcessMemory(hProcess,
                           reinterpret_cast<const void*>(playerBase + kLoadRawRecordRva),
                           &loadRaw.value, sizeof(loadRaw.value), nullptr) ||
        optimizeMin != 0 || loadRaw.value != 0) {
        return LegacyBytecodeFlagState::WriteFailed;
    }
    return LegacyBytecodeFlagState::Applied;
}

void InstallHooksFromInjector(HANDLE hProcess, uintptr_t dllBase,
                              const std::vector<uint8_t>& dllData,
                              RobloxEra era)
{
    printf("InstallHooks: start, dllBase=0x%llX, dllSize=%zu\n", (unsigned long long)dllBase, dllData.size());
    fflush(stdout);

    // 1. Find the hook table in the dedicated .hkinfo PE section
    auto* dd = (const IMAGE_DOS_HEADER*)dllData.data();
    auto* nd = (const IMAGE_NT_HEADERS64*)(dllData.data() + dd->e_lfanew);
    auto* sh = IMAGE_FIRST_SECTION(nd);
    printf("InstallHooks: %u sections\n", nd->FileHeader.NumberOfSections);
    fflush(stdout);

    uintptr_t sectionAddr = 0;
    SIZE_T sectionSize = 0;
    for (WORD i = 0; i < nd->FileHeader.NumberOfSections; i++, sh++) {
        char nm[9] = {}; memcpy(nm, sh->Name, 8);
        printf("InstallHooks: section %d: %.8s va=0x%X size=0x%X\n", i, nm, sh->VirtualAddress, sh->Misc.VirtualSize);
        fflush(stdout);
        if (strcmp(nm, ".hkinfo") == 0) {
            sectionAddr = dllBase + sh->VirtualAddress;
            sectionSize = sh->Misc.VirtualSize;
            printf("InstallHooks: found .hkinfo at 0x%llX size=0x%zX\n", (unsigned long long)sectionAddr, sectionSize);
            fflush(stdout);
            break;
        }
    }
    if (!sectionAddr) { printf("InstallHooks: .hkinfo section not found\n"); return; }

    // Read the HookInfoBlock from remote process
    const uint64_t magic = 0x4B4F4F484E4F4F42ULL;
    HookInfoBlock info = {};
    if (!ReadProcessMemory(hProcess, (void*)sectionAddr, &info, sizeof(info), nullptr)) {
        printf("InstallHooks: failed to read HookInfoBlock (err=%lu)\n", GetLastError());
        return;
    }
    if (info.magic != magic) {
        printf("InstallHooks: bad magic 0x%llX (expected 0x%llX)\n", 
            (unsigned long long)info.magic, (unsigned long long)magic);
        return;
    }
    printf("InstallHooks: %d hooks in table\n", info.count);
    fflush(stdout);

    const bool iatOnly = false;

    // Write JMP hooks for each entry
    int enabled = 0;
    for (int i = 0; i < info.count && i < 16; i++) {
        printf("InstallHooks: hook[%d] %s!%s detour=0x%p\n", 
            i, info.table[i].targetModule, info.table[i].targetFunc, info.table[i].detourFunc);
        fflush(stdout);

        uintptr_t detourAddr = reinterpret_cast<uintptr_t>(info.table[i].detourFunc);
        if (!detourAddr) {
            printf("InstallHooks: %s!%s detour is NULL\n",
                   info.table[i].targetModule, info.table[i].targetFunc);
            continue;
        }

        HMODULE hMod = GetModuleHandleA(info.table[i].targetModule);
        if (!hMod) hMod = LoadLibraryA(info.table[i].targetModule);
        if (!hMod) { printf("InstallHooks: module %s not found locally\n", info.table[i].targetModule); continue; }

        uintptr_t localTarget = (uintptr_t)GetProcAddress(hMod, info.table[i].targetFunc);
        if (!localTarget) { printf("InstallHooks: %s!%s not found\n", info.table[i].targetModule, info.table[i].targetFunc); continue; }

        wchar_t moduleName[MAX_PATH] = {};
        MultiByteToWideChar(CP_ACP, 0, info.table[i].targetModule, -1, moduleName, _countof(moduleName));
        uintptr_t remoteModule = GetRemoteModuleBase(hProcess, moduleName);
        if (!remoteModule) {
            printf("InstallHooks: module %s is not loaded in the target\n", info.table[i].targetModule);
            continue;
        }
        // GetProcAddress can follow a forwarded export (Kernel32!CreateFileW commonly lands in
        // KernelBase).  Resolve the module that actually owns the returned address before applying
        // its RVA in the target process.
        HMODULE localOwner = nullptr;
        if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               reinterpret_cast<LPCSTR>(localTarget), &localOwner)) {
            printf("InstallHooks: cannot identify owner of %s!%s\n",
                   info.table[i].targetModule, info.table[i].targetFunc);
            continue;
        }
        wchar_t ownerName[MAX_PATH] = {};
        if (!GetModuleBaseNameW(GetCurrentProcess(), localOwner, ownerName, _countof(ownerName))) {
            printf("InstallHooks: cannot get owner name of %s!%s\n",
                   info.table[i].targetModule, info.table[i].targetFunc);
            continue;
        }
        uintptr_t remoteOwner = GetRemoteModuleBase(hProcess, ownerName);
        if (!remoteOwner) {
            printf("InstallHooks: resolved owner %ls is not loaded in target\n", ownerName);
            continue;
        }
        uintptr_t targetAddr = remoteOwner + (localTarget - reinterpret_cast<uintptr_t>(localOwner));

        auto enableThroughIat = [&]() {
            size_t iatPatched = 0;
            size_t fallbackPatched = InstallIatFallback(hProcess, dllBase,
                                                   info.table[i].targetModule,
                                                   info.table[i].targetFunc,
                                                   targetAddr, detourAddr);
            if (fallbackPatched == (std::numeric_limits<size_t>::max)()) {
                printf("InstallHooks: aborting %s!%s IAT fallback after an inconsistent slot write\n",
                       info.table[i].targetModule, info.table[i].targetFunc);
                return false;
            }
            iatPatched += fallbackPatched;
            size_t privatePatched = 0;
            if (!iatOnly) {
                privatePatched = PatchRemotePrivateSocketPointers(
                    hProcess, info.table[i].targetFunc, targetAddr, detourAddr);
                if (privatePatched == (std::numeric_limits<size_t>::max)()) {
                    printf("InstallHooks: aborting %s!%s private-pointer fallback after an inconsistent write\n",
                           info.table[i].targetModule, info.table[i].targetFunc);
                    return false;
                }
            }
            iatPatched += privatePatched;
            if (iatPatched > 0) {
                printf("InstallHooks: %s!%s enabled through %zu Roblox-owned pointer slot(s)\n",
                       info.table[i].targetModule, info.table[i].targetFunc, iatPatched);
                enabled++;
                return true;
            }
            printf("InstallHooks: no matching non-system IAT slots for %s!%s\n",
                   info.table[i].targetModule, info.table[i].targetFunc);
            return false;
        };

        // Keep Hyperion and shared system-code pages pristine on 0.728.  The
        // detours still execute inside the Player; only their call sites are
        // redirected through Roblox-owned import tables.
        if (iatOnly) {
            enableThroughIat();
            continue;
        }

        // Compute relative JMP: jmp rel32. Refuse to truncate the displacement; a wrapped jump
        // silently lands in unrelated code and kills the Player on its first network request.
        const int64_t rel64 = static_cast<int64_t>(detourAddr) - static_cast<int64_t>(targetAddr + 5);
        if (rel64 < (std::numeric_limits<int32_t>::min)() ||
            rel64 > (std::numeric_limits<int32_t>::max)()) {
            printf("InstallHooks: %s!%s is outside rel32 range (delta=%lld)\n",
                info.table[i].targetModule, info.table[i].targetFunc, (long long)rel64);
            enableThroughIat();
            continue;
        }
        int32_t rel = static_cast<int32_t>(rel64);

        uint8_t jmp[5] = { 0xE9, (uint8_t)rel, (uint8_t)(rel >> 8), (uint8_t)(rel >> 16), (uint8_t)(rel >> 24) };

        uint8_t originalBytes[sizeof(jmp)] = {};
        SIZE_T originalRead = 0;
        if (!ReadProcessMemory(hProcess, reinterpret_cast<const void*>(targetAddr),
                               originalBytes, sizeof(originalBytes), &originalRead) ||
            originalRead != sizeof(originalBytes)) {
            printf("InstallHooks: cannot snapshot %s!%s prologue; using IAT fallback\n",
                   info.table[i].targetModule, info.table[i].targetFunc);
            enableThroughIat();
            continue;
        }

        printf("InstallHooks: %s!%s at 0x%llX -> 0x%llX (rel=%08X)... ",
            info.table[i].targetModule, info.table[i].targetFunc,
            (unsigned long long)targetAddr, (unsigned long long)detourAddr, rel);

        SYSTEM_INFO systemInfo = {};
        GetSystemInfo(&systemInfo);
        const uintptr_t pageMask = static_cast<uintptr_t>(systemInfo.dwPageSize - 1);
        void* protectBase = reinterpret_cast<void*>(targetAddr & ~pageMask);
        SIZE_T protectSize = ((targetAddr + sizeof(jmp) + pageMask) & ~pageMask) -
                             reinterpret_cast<uintptr_t>(protectBase);
        ULONG oldProtect = 0;
        NTSTATUS protectStatus = SysNtProtectVirtualMemory(
            hProcess, &protectBase, &protectSize, PAGE_EXECUTE_READWRITE, &oldProtect);
        if (protectStatus < 0) {
            printf("NtProtectVirtualMemory failed (status=0x%lX)\n", (unsigned long)protectStatus);
            enableThroughIat();
            continue;
        }

        SIZE_T written = 0;
        NTSTATUS writeStatus = SysNtWriteVirtualMemory(
            hProcess, reinterpret_cast<void*>(targetAddr), jmp, sizeof(jmp), &written);

        uint8_t observedBytes[sizeof(jmp)] = {};
        SIZE_T observedRead = 0;
        const bool installed = writeStatus >= 0 && written == sizeof(jmp) &&
            ReadProcessMemory(hProcess, reinterpret_cast<const void*>(targetAddr),
                              observedBytes, sizeof(observedBytes), &observedRead) &&
            observedRead == sizeof(observedBytes) && memcmp(observedBytes, jmp, sizeof(jmp)) == 0;

        bool rolledBack = true;
        if (!installed) {
            SIZE_T restored = 0;
            NTSTATUS restoreStatus = SysNtWriteVirtualMemory(
                hProcess, reinterpret_cast<void*>(targetAddr), originalBytes, sizeof(originalBytes), &restored);
            memset(observedBytes, 0, sizeof(observedBytes));
            observedRead = 0;
            rolledBack = restoreStatus >= 0 && restored == sizeof(originalBytes) &&
                ReadProcessMemory(hProcess, reinterpret_cast<const void*>(targetAddr),
                                  observedBytes, sizeof(observedBytes), &observedRead) &&
                observedRead == sizeof(observedBytes) &&
                memcmp(observedBytes, originalBytes, sizeof(originalBytes)) == 0;
        }

        ULONG ignoredProtect = 0;
        SysNtProtectVirtualMemory(hProcess, &protectBase, &protectSize, oldProtect, &ignoredProtect);
        FlushInstructionCache(hProcess, reinterpret_cast<void*>(targetAddr), sizeof(jmp));

        if (installed) {
            printf("OK\n");
            enabled++;
        } else {
            printf("NtWriteVirtualMemory failed or did not verify (status=0x%lX, wrote=%zu, rollback=%s)\n",
                (unsigned long)writeStatus, written, rolledBack ? "ok" : "FAILED");
            if (rolledBack)
                enableThroughIat();
            else
                printf("InstallHooks: refusing IAT fallback while %s!%s has an inconsistent prologue\n",
                       info.table[i].targetModule, info.table[i].targetFunc);
        }
    }
    printf("InstallHooks: enabled %d/%d hooks\n", enabled, info.count);
}
