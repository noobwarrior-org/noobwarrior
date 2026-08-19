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
// File: Injector.cpp
// Started by: Hattozo
// Started on: 3/17/2025
// Description: Bootstrapper for launching Roblox w/ noobHook
// The Hyperion path was bruteforced through AI slop
#include <windows.h>

#if defined(_M_X64)
#include "Bypass.h"
#include "ManualMap.h"
#include "Patches.h"
#include "Syscalls.hpp"
#endif

#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <cstdio>
#include <format>
#include <cstdint>
#include <chrono>
#include <cstring>
#include <limits>
#include <psapi.h>

// make sure this matches EngineLaunchResponse in NoobWarrior/Engine.h
enum class EngineLaunchResponse {
    Success,
    Failed,
    NotInstalled,
    NoValidExecutable,
    FailedToCreateProcess,
    InjectFailed,
    InjectDllMissing,
    InjectCannotAccessProcess,
    InjectWrongArchitecture,
    InjectCannotWriteToProcessMemory,
    InjectFailedToGetModuleHandle,
    InjectFailedToGetFunctionAddress,
    InjectCannotCreateThreadInProcess,
    InjectThreadTimedOut,
    InjectCouldNotGetReturnValueOfLoadLibrary,
    InjectFailedToLoadLibrary,
    InjectFailedToResumeProcess,
    WineMissing,
    FailedToLoadPlace
};

static std::string LastErrorStr(DWORD err = GetLastError()) {
    char buf[512] = {0};
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   buf, (DWORD)sizeof(buf), nullptr);
    return std::string(buf);
}

#if defined(_M_X64)
struct PatchProbe {
    uintptr_t address;
    uintptr_t rva;
    uint8_t intendedFirstByte;
    size_t hitOrder;
};

static std::vector<PatchProbe> InstallPatchProbes(HANDLE process,
                                                  uintptr_t moduleBase,
                                                  RobloxEra era) {
    std::vector<PatchProbe> probes;
    for (const auto& patch : GetPatchesForEra(era)) {
        if (patch.bytes.empty())
            continue;
        const uintptr_t address = moduleBase + patch.rva;
        DWORD oldProtection = 0;
        if (!VirtualProtectEx(process, reinterpret_cast<void*>(address), 1,
                              PAGE_EXECUTE_READWRITE, &oldProtection))
            continue;
        const uint8_t breakpoint = 0xCC;
        SIZE_T written = 0;
        const bool installed = WriteProcessMemory(
            process, reinterpret_cast<void*>(address), &breakpoint, 1, &written) &&
            written == 1;
        DWORD ignored = 0;
        VirtualProtectEx(process, reinterpret_cast<void*>(address), 1,
                         oldProtection, &ignored);
        if (installed)
            probes.push_back({address, patch.rva, patch.bytes[0], 0});
    }
    FlushInstructionCache(process, nullptr, 0);
    printf("Installed %zu one-shot patch execution probes\n", probes.size());
    return probes;
}

static bool ObserveDebugExceptions(HANDLE process, DWORD processId,
                                   DWORD timeoutMilliseconds,
                                   bool resumeAfterAttach,
                                   std::vector<PatchProbe>* probes = nullptr) {
    if (!DebugActiveProcess(processId)) {
        const DWORD error = GetLastError();
        printf("Debug observer attach failed (%lu: %s)\n",
               error, LastErrorStr(error).c_str());
        return false;
    }
    DebugSetProcessKillOnExit(FALSE);
    printf("Debug observer attached to PID %lu\n", processId);
    if (resumeAfterAttach) {
        ResumeProcess(processId);
        printf("Debug observer resumed PID %lu after attach\n", processId);
    }

    const ULONGLONG deadline = GetTickCount64() + timeoutMilliseconds;
    bool processExited = false;
    size_t nextProbeOrder = 1;
    while (!processExited && GetTickCount64() < deadline) {
        DEBUG_EVENT event = {};
        const ULONGLONG now = GetTickCount64();
        const DWORD remaining = now < deadline
            ? static_cast<DWORD>((std::min<ULONGLONG>)(deadline - now, 500))
            : 0;
        if (!WaitForDebugEvent(&event, remaining)) {
            if (GetLastError() != ERROR_SEM_TIMEOUT)
                printf("WaitForDebugEvent failed (%lu)\n", GetLastError());
            continue;
        }

        DWORD continuation = DBG_CONTINUE;
        switch (event.dwDebugEventCode) {
        case CREATE_PROCESS_DEBUG_EVENT:
            if (event.u.CreateProcessInfo.hFile)
                CloseHandle(event.u.CreateProcessInfo.hFile);
            break;
        case LOAD_DLL_DEBUG_EVENT:
            if (event.u.LoadDll.hFile)
                CloseHandle(event.u.LoadDll.hFile);
            break;
        case EXCEPTION_DEBUG_EVENT: {
            const auto& record = event.u.Exception.ExceptionRecord;
            const bool firstChance = event.u.Exception.dwFirstChance != 0;
            PatchProbe* hitProbe = nullptr;
            if (probes && record.ExceptionCode == EXCEPTION_BREAKPOINT) {
                const uintptr_t exceptionAddress =
                    reinterpret_cast<uintptr_t>(record.ExceptionAddress);
                for (auto& probe : *probes) {
                    if (!probe.hitOrder && probe.address == exceptionAddress) {
                        hitProbe = &probe;
                        break;
                    }
                }
            }

            if (hitProbe) {
                DWORD oldProtection = 0;
                VirtualProtectEx(process, reinterpret_cast<void*>(hitProbe->address), 1,
                                 PAGE_EXECUTE_READWRITE, &oldProtection);
                SIZE_T written = 0;
                WriteProcessMemory(process, reinterpret_cast<void*>(hitProbe->address),
                                   &hitProbe->intendedFirstByte, 1, &written);
                DWORD ignored = 0;
                VirtualProtectEx(process, reinterpret_cast<void*>(hitProbe->address), 1,
                                 oldProtection, &ignored);
                FlushInstructionCache(process,
                                      reinterpret_cast<const void*>(hitProbe->address), 1);
                hitProbe->hitOrder = nextProbeOrder++;
                printf("Patch probe hit #%zu RVA=0x%llX address=0x%llX\n",
                       hitProbe->hitOrder,
                       (unsigned long long)hitProbe->rva,
                       (unsigned long long)hitProbe->address);
            }

            printf("Debug exception code=0x%08lX address=0x%llX firstChance=%s thread=%lu",
                   record.ExceptionCode,
                   (unsigned long long)reinterpret_cast<uintptr_t>(record.ExceptionAddress),
                   firstChance ? "yes" : "no", event.dwThreadId);
            if (record.ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
                record.NumberParameters >= 2) {
                printf(" operation=%llu target=0x%llX",
                       (unsigned long long)record.ExceptionInformation[0],
                       (unsigned long long)record.ExceptionInformation[1]);
            }
            printf("\n");

            HANDLE thread = OpenThread(THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION,
                                       FALSE, event.dwThreadId);
            if (thread) {
                CONTEXT context = {};
                context.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
                if (GetThreadContext(thread, &context)) {
                    if (hitProbe) {
                        context.Rip = hitProbe->address;
                        SetThreadContext(thread, &context);
                    }
                    printf("Debug context RIP=0x%llX RSP=0x%llX RBP=0x%llX "
                           "RAX=0x%llX RCX=0x%llX RDX=0x%llX\n",
                           (unsigned long long)context.Rip,
                           (unsigned long long)context.Rsp,
                           (unsigned long long)context.Rbp,
                           (unsigned long long)context.Rax,
                           (unsigned long long)context.Rcx,
                           (unsigned long long)context.Rdx);
                }
                CloseHandle(thread);
            }

            continuation = record.ExceptionCode == EXCEPTION_BREAKPOINT
                ? DBG_CONTINUE : DBG_EXCEPTION_NOT_HANDLED;
            break;
        }
        case EXIT_PROCESS_DEBUG_EVENT:
            printf("Debug observer saw process exit status 0x%08lX\n",
                   event.u.ExitProcess.dwExitCode);
            processExited = true;
            break;
        default:
            break;
        }
        ContinueDebugEvent(event.dwProcessId, event.dwThreadId, continuation);
    }

    if (!processExited) {
        DebugActiveProcessStop(processId);
        printf("Debug observer detached after %lu ms\n", timeoutMilliseconds);
    }
    if (probes) {
        printf("Patch probes executed: %zu/%zu\n", nextProbeOrder - 1, probes->size());
        for (const auto& probe : *probes) {
            if (probe.hitOrder)
                printf("Patch probe order #%zu RVA=0x%llX\n",
                       probe.hitOrder, (unsigned long long)probe.rva);
        }
    }
    return true;
}

#endif


struct SavedEnvironmentVariable {
    bool existed = false;
    std::wstring value;
};

static SavedEnvironmentVariable SaveEnvironmentVariable(const wchar_t* name) {
    SavedEnvironmentVariable saved;
    DWORD needed = GetEnvironmentVariableW(name, nullptr, 0);
    if (needed == 0)
        return saved;
    std::vector<wchar_t> value(needed);
    if (GetEnvironmentVariableW(name, value.data(), needed) != 0) {
        saved.existed = true;
        saved.value = value.data();
    }
    return saved;
}

static void RestoreEnvironmentVariable(const wchar_t* name, const SavedEnvironmentVariable& saved) {
    SetEnvironmentVariableW(name, saved.existed ? saved.value.c_str() : nullptr);
}

static bool IsProcess64Bit(HANDLE hProcess) {
    BOOL isWow64 = FALSE;
    IsWow64Process(hProcess, &isWow64);
    SYSTEM_INFO si;
    GetNativeSystemInfo(&si);
    if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
        return !isWow64;
    return false;
}

static int GetDllBitness(const wchar_t* dllPath) {
#if defined(_WIN32)
    HANDLE hFile = CreateFileW(dllPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return -1;
    HANDLE hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (hMap == NULL) { CloseHandle(hFile); return -1; }
    LPVOID lpBase = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    if (lpBase == NULL) { CloseHandle(hMap); CloseHandle(hFile); return -1; }

    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)lpBase;
    if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        UnmapViewOfFile(lpBase); CloseHandle(hMap); CloseHandle(hFile); return -1;
    }
    PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((BYTE*)lpBase + pDosHeader->e_lfanew);
    if (pNtHeaders->Signature != IMAGE_NT_SIGNATURE) {
        UnmapViewOfFile(lpBase); CloseHandle(hMap); CloseHandle(hFile); return -1;
    }
    int bitness = 0;
    switch (pNtHeaders->FileHeader.Machine) {
    case IMAGE_FILE_MACHINE_I386:  bitness = 32; break;
    case IMAGE_FILE_MACHINE_AMD64: bitness = 64; break;
    default:                       bitness = 0;  break;
    }
    UnmapViewOfFile(lpBase); CloseHandle(hMap); CloseHandle(hFile);
    return bitness;
#else
    return -1;
#endif
}

static std::filesystem::path GetExeDirectory() {
    wchar_t buffer[MAX_PATH];
    DWORD length = GetModuleFileNameW(NULL, buffer, MAX_PATH);
    if (length == 0) return "";
    return std::filesystem::path(buffer).parent_path();
}

static void QuarantineRobloxFlagCache() {
    wchar_t tempPath[MAX_PATH] = {};
    const DWORD length = GetTempPathW(_countof(tempPath), tempPath);
    if (length == 0 || length >= _countof(tempPath)) {
        printf("Cannot locate Roblox flag cache (GetTempPathW error %lu)\n", GetLastError());
        return;
    }

    const std::filesystem::path cacheDirectory =
        std::filesystem::path(tempPath) / L"Roblox" / L"cache";
    const std::filesystem::path backupDirectory =
        cacheDirectory / L"noobwarrior-backup";
    std::error_code error;
    if (!std::filesystem::exists(cacheDirectory, error))
        return;

    std::filesystem::create_directories(backupDirectory, error);
    if (error) {
        printf("Cannot create flag-cache backup directory: %s\n", error.message().c_str());
        return;
    }

    constexpr const wchar_t* kCacheFiles[] = {
        L"flag_cache.dat",
        L"flag_cache_backup.dat",
        L"tombstone.dat",
    };
    for (const wchar_t* name : kCacheFiles) {
        const std::filesystem::path source = cacheDirectory / name;
        if (!std::filesystem::exists(source, error)) {
            error.clear();
            continue;
        }

        const std::filesystem::path backup =
            backupDirectory / (std::wstring(name) + L".pre-noobwarrior");
        if (!std::filesystem::exists(backup, error)) {
            error.clear();
            std::filesystem::copy_file(source, backup,
                                       std::filesystem::copy_options::none, error);
            if (error) {
                printf("Cannot preserve stale flag cache %ls: %s\n",
                       source.c_str(), error.message().c_str());
                error.clear();
                continue;
            }
        }

        std::filesystem::remove(source, error);
        if (error) {
            printf("Cannot quarantine stale flag cache %ls: %s\n",
                   source.c_str(), error.message().c_str());
            error.clear();
        } else {
            printf("Quarantined stale Roblox flag cache: %ls\n", source.c_str());
        }
    }
}

static EngineLaunchResponse Inject(unsigned long pid, const wchar_t *dllPath) {
    EngineLaunchResponse res = EngineLaunchResponse::InjectFailed;

    if (!std::filesystem::exists(dllPath)) {
        printf("DLL does not exist: %ls\n", std::filesystem::path(dllPath).c_str());
        return EngineLaunchResponse::InjectDllMissing;
    }

    printf("Starting injection - PID: %lu, DLL: %ls\n", pid, std::filesystem::path(dllPath).c_str());
    HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (handle == NULL) {
        DWORD err = GetLastError();
        printf("OpenProcess failed: %lu (%s)\n", err, LastErrorStr(err).c_str());
        return EngineLaunchResponse::InjectCannotAccessProcess;
    }

    BOOL is64Bit = IsProcess64Bit(handle);
    int dllBitness = GetDllBitness(dllPath);
    DWORD exitCode = 0;

    printf("Target 64 bit: %s, DLL bitness: %i\n", is64Bit ? "yes" : "no", dllBitness);
    if (is64Bit && dllBitness == 32) {
        printf("Error, architecture mismatch\n");
        CloseHandle(handle);
        return EngineLaunchResponse::InjectWrongArchitecture;
    }

    wchar_t absPath[MAX_PATH];
    GetFullPathNameW(dllPath, MAX_PATH, absPath, nullptr);

    void *mem = VirtualAllocEx(handle, NULL, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!mem) {
        printf("VirtualAllocEx failed: %lu\n", GetLastError());
        CloseHandle(handle);
        return EngineLaunchResponse::InjectCannotWriteToProcessMemory;
    }

    if (!WriteProcessMemory(handle, mem, absPath, MAX_PATH, NULL)) {
        printf("WriteProcessMemory failed: %lu\n", GetLastError());
        VirtualFreeEx(handle, mem, 0, MEM_RELEASE);
        CloseHandle(handle);
        return EngineLaunchResponse::InjectCannotWriteToProcessMemory;
    }

    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!hKernel32) {
        printf("Failed to get kernel32 handle\n");
        VirtualFreeEx(handle, mem, 0, MEM_RELEASE);
        CloseHandle(handle);
        return EngineLaunchResponse::InjectFailedToGetModuleHandle;
    }

    FARPROC pLoadLibraryW = GetProcAddress(hKernel32, "LoadLibraryW");
    if (!pLoadLibraryW) {
        printf("Failed to get LoadLibraryW address\n");
        VirtualFreeEx(handle, mem, 0, MEM_RELEASE);
        CloseHandle(handle);
        return EngineLaunchResponse::InjectFailedToGetFunctionAddress;
    }

    HANDLE thread;
    thread = CreateRemoteThread(handle, nullptr, 0, (LPTHREAD_START_ROUTINE)pLoadLibraryW, mem, 0, nullptr);
    if (thread == nullptr) {
        res = EngineLaunchResponse::InjectCannotCreateThreadInProcess;
        goto cleanup;
    }
    if (WaitForSingleObject(thread, 120000) != WAIT_OBJECT_0) {
        res = EngineLaunchResponse::InjectThreadTimedOut;
        goto cleanup;
    }

    GetExitCodeThread(thread, &exitCode);
    printf("LoadLibraryW returned 0x%llX\n", (unsigned long long)exitCode);
    {
        bool looksValid = (exitCode != 0);
        if (looksValid) {
            ULONG_PTR addr = (ULONG_PTR)exitCode;
            if ((addr & 0xFFFF0000) == 0xC0000000 || (addr & 0xFFFF0000) == 0x80000000)
                looksValid = false;
        }
        if (!looksValid) {
            printf("LoadLibraryW returned error code 0x%llX\n", (unsigned long long)exitCode);
            res = EngineLaunchResponse::InjectFailedToLoadLibrary;
            goto cleanup;
        }
    }
    res = EngineLaunchResponse::Success;
cleanup:
    if (mem) VirtualFreeEx(handle, mem, 0, MEM_RELEASE);
    if (thread) CloseHandle(thread);
    CloseHandle(handle);
    return res;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int showCmd) {
    std::filesystem::path exeDir = GetExeDirectory();

    std::filesystem::path logDir = exeDir / "noobhook_injector.log";
    std::string logDirStr = logDir.string();

    std::filesystem::path dllx86Dir = exeDir / "noobhook_x86.dll";
    std::wstring dllx86DirStr = dllx86Dir.wstring();

    std::filesystem::path dllx64Dir = exeDir / "noobhook_x86-64.dll";
    std::wstring dllx64DirStr = dllx64Dir.wstring();

    std::filesystem::path dllx64HyperionDir = exeDir / "noobhook_x86-64_hyperion.dll";
    std::wstring dllx64HyperionStr = dllx64HyperionDir.wstring();

    std::filesystem::path dllx64BootstrapDir = exeDir / "noobhook_x86-64_hyperion_bootstrap.dll";
    std::wstring dllx64BootstrapStr = dllx64BootstrapDir.wstring();

    // Keep consecutive Player and Studio launches in one trace. Studio may be
    // launched immediately after Player and previously erased the only record
    // of an early Player exit by reopening this file with "w".
    FILE* file = freopen(logDirStr.c_str(), "a", stdout);
    if (file != nullptr) {
        setvbuf(file, nullptr, _IONBF, 0);
        printf("\n=== noobHook Injector pid=%lu ===\n", GetCurrentProcessId());
    }

#if defined(_M_X64)
    if (!InitSyscalls()) { printf("Syscall init failed\n"); return -1; }
#endif

    int argc;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    if (argc == 1) {
        MessageBoxA(nullptr,
            R"(Note: This program is not meant to be used on its own.
Args:
--file: path to RobloxPlayerBeta.exe / RCCService.exe / RobloxStudioBeta.exe
--ip: server IP (default: 127.0.0.1)
--port: server port (default: 53640)
--placeid: place ID (default: 1818)
--emuhttp: emulator HTTP port
--emuhttps: emulator HTTPS port
--emucert: path to emulator CA cert
--side: "server", "client", or "studio"
--scheme: "home" (app UI), "app" (modern join), "new" (2023 --play), or "old" (legacy)
--authticket: launch auth ticket (defaults to "1"))",
            "noobHook Injector", MB_ICONINFORMATION | MB_OK);
        return 0;
    }

    std::wstring filePathStr, ipStr, portStr, placeIdStr;
    std::wstring emuHttpStr, emuHttpsStr, sideStr, schemeStr, emuCertStr, authTicketStr;
    for (int i = 0; i < argc; i++) {
        if (i + 1 >= argc) break;
        if (wcscmp(argv[i], L"--file") == 0)      filePathStr = argv[i + 1];
        if (wcscmp(argv[i], L"--ip") == 0)        ipStr = argv[i + 1];
        if (wcscmp(argv[i], L"--port") == 0)      portStr = argv[i + 1];
        if (wcscmp(argv[i], L"--placeid") == 0)   placeIdStr = argv[i + 1];
        if (wcscmp(argv[i], L"--emuhttp") == 0)   emuHttpStr = argv[i + 1];
        if (wcscmp(argv[i], L"--emuhttps") == 0)  emuHttpsStr = argv[i + 1];
        if (wcscmp(argv[i], L"--side") == 0)      sideStr = argv[i + 1];
        if (wcscmp(argv[i], L"--scheme") == 0)    schemeStr = argv[i + 1];
        if (wcscmp(argv[i], L"--emucert") == 0)   emuCertStr = argv[i + 1];
        if (wcscmp(argv[i], L"--authticket") == 0) authTicketStr = argv[i + 1];
    }

    std::filesystem::path filePath(filePathStr);
    if (filePathStr.empty()) { printf("No --file argument\n"); return 1; }

    std::filesystem::path fileName = filePath.filename();
    std::filesystem::path fileDir = filePath.parent_path();
    const std::filesystem::path sourceEngineDir = fileDir;

#if defined(_M_X64)
    // Older launcher binaries categorize every pre-0.728 client as the 2023
    // `new` scheme. Give Player 0.719 a dedicated direct-join path here as
    // well as in Core so Direct Connect works immediately with either
    // launcher: explicit server arguments mean join; no target means the
    // ordinary app/home UI.
    if (_wcsicmp(fileName.c_str(), L"RobloxPlayerBeta.exe") == 0 &&
        UsesEarlyWspRedirect(DetectEra(filePathStr.c_str())) && schemeStr == L"new") {
        const bool hasJoinTarget = !ipStr.empty() || !portStr.empty() || !placeIdStr.empty();
        schemeStr = hasJoinTarget ? L"app" : L"home";
        printf("Normalized legacy launcher scheme for Player 0.719 -> %ls\n",
               schemeStr.c_str());
    }
#endif

    // Core has already installed the merged, backed-up bundle beside an app-mode Player.
    const bool usesMergedPlayerCa =
        _wcsicmp(fileName.c_str(), L"RobloxPlayerBeta.exe") == 0 &&
        (schemeStr == L"app" || schemeStr == L"home");
    std::filesystem::path playerCaBundle;
    if (usesMergedPlayerCa)
        playerCaBundle = sourceEngineDir / L"ssl" / L"cacert.pem";
    std::wstring fileDirStr = fileDir.wstring();

    std::wstring wargs = L"\"" + filePathStr + L"\"";
    if (fileName.compare("RCCService.exe") == 0) {
        std::wstring rccPlaceId = placeIdStr.empty() ? L"1818" : placeIdStr;
        wargs += std::format(L" -console -verbose -placeid:{} -port 53641 -localtest \"gameserver.json\" -settingsfile \"DevSettingsFile.json\"", rccPlaceId);
    } else if (fileName.compare("RobloxPlayerBeta.exe") == 0) {
        std::wstring ticket = authTicketStr.empty() ? L"1" : authTicketStr;
        if (schemeStr == L"home") {
            wargs += L" --app";
            printf("Player launch mode: --app home\n");
        } else if (schemeStr == L"app" || schemeStr == L"new") {
            std::wstring placeId = placeIdStr.empty() ? L"1818" : placeIdStr;
            std::wstring ip = ipStr.empty() ? L"127.0.0.1" : ipStr;
            std::wstring port = portStr.empty() ? L"53640" : portStr;
            std::wstring placeLauncher = std::format(L"http://www.roblox.com/Game/PlaceLauncher.ashx?request=RequestGame&ip={}&port={}&placeId={}", ip, port, placeId);
            if (schemeStr == L"app") {
                const auto launchTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                std::wstring deeplink = std::format(
                    L"roblox://experiences/start?placeId={}", placeId);
#if defined(_M_X64)
                if (UsesEarlyWspRedirect(DetectEra(filePathStr.c_str()))) {
                    wargs += std::format(
                        L" --app -t \"{}\" --launchtime={} -j \"{}\" -b \"12345678\" --rloc en_us --gloc en_us",
                        ticket, launchTime, placeLauncher);
                    printf("Player launch mode: 0.719 native --app join (no deeplink), PlaceLauncher: %ls\n",
                           placeLauncher.c_str());
                } else
#endif
                {
                    wargs += std::format(L" --app -t \"{}\" --launchtime={} -j \"{}\" -b \"12345678\" --rloc en_us --gloc en_us --deeplink \"{}\"",
                                         ticket, launchTime, placeLauncher, deeplink);
                    printf("Player launch mode: --app startup deeplink (%ls), PlaceLauncher: %ls\n",
                           deeplink.c_str(), placeLauncher.c_str());
                }
            } else {
                std::wstring deeplink = std::format(
                    L"roblox://experiences/start?placeId={}", placeId);
                // The 2023 client requires the older protocol-launcher spelling.
                wargs += std::format(L" --play -b \"12345678\" -t \"{}\" --launchtime 1716000000000 --rloc en_us --gloc en_us --deeplink \"{}\" -j \"{}\"",
                                     ticket, deeplink, placeLauncher);
                printf("Player launch mode: --play, PlaceLauncher: %ls\n", placeLauncher.c_str());
            }
        } else {
            wargs += std::format(L" -a \"http://www.roblox.com/Login/Negotiate.ashx\" -j \"http://www.roblox.com/Game/PlaceLauncher.ashx?ip={}&port={}&placeId={}\" -t \"{}\"", ipStr, portStr, placeIdStr, ticket);
        }
    } else if (fileName.compare("RobloxStudioBeta.exe") == 0 && sideStr == L"server") {
        std::wstring port = portStr.empty() ? L"53640" : portStr;
        std::wstring placeId = placeIdStr.empty() ? L"1818" : placeIdStr;
        wargs += std::format(L" -task StartServer -port {} -placeId {} -universeId 1 -creatorId 1 -creatorType 0 -placeVersion 1 -numTestServerPlayersUponStartup 0 -instanceId StudioServer -gameId Test -Console", port, placeId);
    }
    std::vector<wchar_t> wargs_vec(wargs.begin(), wargs.end()); wargs_vec.push_back(L'\0');
    std::vector<wchar_t> filedir_vec(fileDirStr.begin(), fileDirStr.end()); filedir_vec.push_back(L'\0');

    if (!emuHttpStr.empty())  SetEnvironmentVariableW(L"NOOBHOOK_HTTP_PORT", emuHttpStr.c_str());
    if (!emuHttpsStr.empty()) SetEnvironmentVariableW(L"NOOBHOOK_HTTPS_PORT", emuHttpsStr.c_str());
    if (!sideStr.empty())     SetEnvironmentVariableW(L"NOOBHOOK_SIDE", sideStr.c_str());
    if (!portStr.empty())     SetEnvironmentVariableW(L"NOOBHOOK_PORT", portStr.c_str());
    if (!placeIdStr.empty())  SetEnvironmentVariableW(L"NOOBHOOK_PLACEID", placeIdStr.c_str());
    // The installed app-mode bundle is already merged. Keep noobhook's older temporary redirect
    // available only to the legacy/new launch paths.
    if (usesMergedPlayerCa && !SetEnvironmentVariableW(L"NOOBHOOK_EMU_CERT", nullptr)) {
        printf("Cannot disable legacy Player CA redirect (error %lu)\n", GetLastError());
        return 7;
    } else if (!usesMergedPlayerCa && !emuCertStr.empty()) {
        SetEnvironmentVariableW(L"NOOBHOOK_EMU_CERT", emuCertStr.c_str());
    }
    if (usesMergedPlayerCa) {
        const std::filesystem::path playerHookLog = sourceEngineDir / L"noobhook.log";
        if (!SetEnvironmentVariableW(L"NOOBHOOK_LOG_PATH", playerHookLog.c_str())) {
            printf("Cannot set Player hook log path (error %lu)\n", GetLastError());
            return 7;
        }
        printf("Player hook log: %ls\n", playerHookLog.c_str());
    }

    PROCESS_INFORMATION pi {};
    STARTUPINFOW si = {}; si.cb = sizeof(si);

    auto createTargetProcess = [&](DWORD creationFlags) -> BOOL {
        SavedEnvironmentVariable savedSslCertFile;
        if (!playerCaBundle.empty()) {
            savedSslCertFile = SaveEnvironmentVariable(L"SSL_CERT_FILE");
            if (!SetEnvironmentVariableW(L"SSL_CERT_FILE", playerCaBundle.c_str())) {
                DWORD environmentError = GetLastError();
                RestoreEnvironmentVariable(L"SSL_CERT_FILE", savedSslCertFile);
                printf("Cannot set Player CA environment (error %lu)\n", environmentError);
                SetLastError(environmentError);
                return FALSE;
            }
        }

        BOOL created = CreateProcessW(filePathStr.c_str(), wargs_vec.data(), nullptr, nullptr, FALSE,
                                      creationFlags, nullptr, filedir_vec.data(), &si, &pi);
        DWORD createError = GetLastError();

        // CreateProcess has already copied the environment. Do not leak Player-specific trust
        // settings into the injector or any process it might launch later.
        if (!playerCaBundle.empty()) {
            RestoreEnvironmentVariable(L"SSL_CERT_FILE", savedSslCertFile);
        }
        SetLastError(createError);
        return created;
    };

#if defined(_M_X64)
    // ---- Hyperion bypass detection ----
    RobloxEra era = DetectEra(filePathStr.c_str());
    printf("Detected Roblox era: %ls\n", EraName(era));
    bool hasByfron = NeedsBypass(era) && HasSectionOnDisk(filePathStr.c_str(), ".byfron");
    if (!hasByfron && NeedsBypass(era)) {
        std::filesystem::path dllPath = filePath.parent_path() / L"RobloxPlayerBeta.dll";
        if (std::filesystem::exists(dllPath) && HasSectionOnDisk(dllPath.c_str(), ".byfron"))
            hasByfron = true;
    }
    printf("Has .byfron section: %s\n", hasByfron ? "yes" : "no");

    if (hasByfron) {
        // ---- Hyperion bypass path ----
        printf("Hyperion bypass required\n");
        const bool useEarlySuspendedBootstrap = UsesEarlyWspRedirect(era);
        DWORD creationFlags = useEarlySuspendedBootstrap ? CREATE_SUSPENDED : 0;

        if (useEarlySuspendedBootstrap && sideStr == L"client")
            QuarantineRobloxFlagCache();

        if (!createTargetProcess(creationFlags)) {
            printf("CreateProcessW failed: %lu (%s)\n", GetLastError(), LastErrorStr().c_str());
            return 7;
        }
        DWORD targetPid = GetProcessId(pi.hProcess);
        uintptr_t earlyBootstrapBase = 0;
        std::vector<uint8_t> earlyBootstrapData;
        if (useEarlySuspendedBootstrap) {
            // Map only the loader-free WSP bridge before the first Player instruction. The view is
            // backed by the bootstrap DLL itself, and its entry point is intentionally not run.
            // Roblox, Hyperion, and the Windows socket DLLs remain byte-for-byte untouched.
            printf("%ls suspended clean-image startup enabled (no debugger)\n", EraName(era));
            earlyBootstrapData = ReadDllFile(dllx64BootstrapStr.c_str());
            wchar_t systemDirectory[MAX_PATH] = {};
            const UINT systemDirectoryLength =
                GetSystemDirectoryW(systemDirectory, _countof(systemDirectory));
            const std::filesystem::path earlyBackingPath =
                systemDirectoryLength > 0 && systemDirectoryLength < _countof(systemDirectory)
                    ? std::filesystem::path(systemDirectory) / L"version.dll"
                    : std::filesystem::path();
            earlyBootstrapBase = earlyBootstrapData.empty() || earlyBackingPath.empty() ? 0 :
                ImageBackedManualMapDll(pi.hProcess, earlyBootstrapData, false,
                                        earlyBackingPath.c_str());
            if (!earlyBootstrapBase) {
                printf("%ls early WSP bootstrap mapping failed\n", EraName(era));
                TerminateProcess(pi.hProcess, 0xEEEEEEE3);
                WaitForSingleObject(pi.hProcess, 5000);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                return 1;
            }
            printf("%ls early WSP bootstrap mapped at 0x%llX while suspended\n",
                   EraName(era), static_cast<unsigned long long>(earlyBootstrapBase));

            if (ResumeThread(pi.hThread) == static_cast<DWORD>(-1)) {
                printf("%ls initial-thread resume failed: %lu\n", EraName(era), GetLastError());
                TerminateProcess(pi.hProcess, 0xEEEEEEE2);
                WaitForSingleObject(pi.hProcess, 5000);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                return 1;
            }
            printf("%ls initial thread resumed; waiting for the first MSWSOCK provider table\n",
                   EraName(era));

            auto parsePort = [](const std::wstring& text, uint16_t fallback) {
                if (text.empty())
                    return fallback;
                wchar_t* end = nullptr;
                const unsigned long value = wcstoul(text.c_str(), &end, 10);
                return end && *end == L'\0' && value > 0 && value <= 65535
                    ? static_cast<uint16_t>(value) : fallback;
            };
            const uint16_t emulatorHttpPort = parsePort(emuHttpStr, 8080);
            const uint16_t emulatorHttpsPort = parsePort(emuHttpsStr, 53640);
            size_t providerTablesPatched = 0;
            DWORD providerWaitMs = 0;
            for (; providerWaitMs < 5000; ++providerWaitMs) {
                if (GetRemoteModuleBase(pi.hProcess, L"mswsock.dll")) {
                    providerTablesPatched = InstallEarlyWspRedirect(
                        pi.hProcess, earlyBootstrapBase, earlyBootstrapData,
                        emulatorHttpPort, emulatorHttpsPort);
                    if (providerTablesPatched != 0)
                        break;
                }
                DWORD exitCode = 0;
                if (GetExitCodeProcess(pi.hProcess, &exitCode) && exitCode != STILL_ACTIVE) {
                    printf("%ls process exited before WSP redirect installation: 0x%08lX\n",
                           EraName(era), exitCode);
                    break;
                }
                Sleep(1);
            }
            if (providerTablesPatched == (std::numeric_limits<size_t>::max)()) {
                printf("%ls WSP provider table write became inconsistent\n", EraName(era));
                providerTablesPatched = 0;
            }
            if (providerTablesPatched == 0) {
                printf("%ls WSP redirect was not installed within %lu ms\n",
                       EraName(era), providerWaitMs);
            } else {
                printf("%ls WSP redirect installed after %lu ms (%zu table)\n",
                       EraName(era), providerWaitMs, providerTablesPatched);
            }
        }

        // The 0.719 guard runs during the DLL's .byfron split. A speculative
        // LoadLibrary attempt loses that race (and always fails under this
        // build), so reach the pre-remap patch as soon as the DLL appears.
        if (UsesEarlyWspRedirect(era)) {
            printf("Skipping pre-bypass LoadLibrary attempt for %ls\n", EraName(era));
        } else {
            const wchar_t* earlyPath = IsProcess64Bit(pi.hProcess)
                ? dllx64HyperionStr.c_str() : dllx86DirStr.c_str();
            printf("Early inject attempt: %ls\n", earlyPath);
            EngineLaunchResponse earlyInject = Inject(targetPid, earlyPath);
            printf("Early inject result: %d\n", (int)earlyInject);
        }

        // Wait for .byfron module to load
        printf("Waiting for .byfron module...\n");
        uintptr_t hyperionBase = 0;
        if (useEarlySuspendedBootstrap) {
            for (int i = 0; i < 15000; ++i) {
                const uintptr_t playerBase =
                    GetRemoteModuleBase(targetPid, L"RobloxPlayerBeta.dll");
                if (playerBase) {
                    hyperionBase = playerBase;
                    printf("  Player DLL observed after %d ms\n", i);
                    break;
                }
                DWORD exitCode = 0;
                if (GetExitCodeProcess(pi.hProcess, &exitCode) && exitCode != STILL_ACTIVE) {
                    printf("  process exited (code %lu) at iteration %d\n", exitCode, i);
                    break;
                }
                Sleep(1);
            }
        } else {
            for (int i = 0; i < 300; i++) {
                hyperionBase = FindModuleWithSection(targetPid, ".byfron");
                if (hyperionBase) { printf("  found at iteration %d\n", i); break; }
                DWORD exitCode = 0;
                if (GetExitCodeProcess(pi.hProcess, &exitCode) && exitCode != STILL_ACTIVE) {
                    printf("  process exited (code %lu) at iteration %d\n", exitCode, i);
                    break;
                }
                Sleep(50);
            }
        }
        if (!hyperionBase) {
            printf("Timed out waiting for .byfron\n");
            TerminateProcess(pi.hProcess, 0xFFFFFFFF);
            WaitForSingleObject(pi.hProcess, 5000);
            CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
            return 1;
        }

        // Detect which module has .byfron (prefer DLL over EXE stub)
        {
            wchar_t modName[MAX_PATH] = {0};
            HANDLE hQ = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, targetPid);
            if (hQ) { GetModuleBaseNameW(hQ, (HMODULE)hyperionBase, modName, MAX_PATH); CloseHandle(hQ); }
            printf(".byfron module at 0x%llX (%ls)\n", (unsigned long long)hyperionBase, modName);
            if (_wcsicmp(modName, L"RobloxPlayerBeta.exe") == 0) {
                uintptr_t dllBase = GetRemoteModuleBase(targetPid, L"RobloxPlayerBeta.dll");
                if (dllBase) hyperionBase = dllBase;
            }
        }

        // 0.719 uses only the loader-free bootstrap installed before its first socket call. The
        // full noobHook payload is intentionally omitted: its dependency loading and DllMain run
        // after the app has already connected, and add a much larger integrity-visible surface.
        if (UsesEarlyWspRedirect(era)) {
            printf("%ls early WSP bootstrap active; full Hyperion hook payload skipped\n",
                   EraName(era));
            DWORD survivalWait = WAIT_TIMEOUT;
            const uintptr_t playerExeBase =
                GetRemoteModuleBase(pi.hProcess, L"RobloxPlayerBeta.exe");
            bool reportedWaitingForFlags = false;
            bool reportedFlagsApplied = false;
            bool reportedFlagWriteFailure = false;
            const DWORD statTimes[] = {2000, 5000, 10000, 15000};
            size_t nextStat = 0;
            DWORD elapsed = 0;
            while (elapsed < 15000) {
                const LegacyBytecodeFlagState flagState =
                    ReassertLegacyBytecodeFlags(pi.hProcess, playerExeBase, era);
                if (flagState == LegacyBytecodeFlagState::WaitingForPlayerInitialization &&
                    !reportedWaitingForFlags) {
                    printf("%ls bytecode FastVariables not initialized yet; retrying\n",
                           EraName(era));
                    reportedWaitingForFlags = true;
                } else if (flagState == LegacyBytecodeFlagState::Applied &&
                           !reportedFlagsApplied) {
                    printf("%ls deterministic bytecode flags active: "
                           "FIntLsbOptimizeMin=0, FFlagLoadRawBytecodeWithHashKey=False\n",
                           EraName(era));
                    reportedFlagsApplied = true;
                } else if (flagState == LegacyBytecodeFlagState::WriteFailed &&
                           !reportedFlagWriteFailure) {
                    printf("%ls bytecode FastVariable write failed; retrying\n", EraName(era));
                    reportedFlagWriteFailure = true;
                }

                const DWORD interval = elapsed < 3000 ? 5 : 100;
                survivalWait = WaitForSingleObject(pi.hProcess, interval);
                if (survivalWait != WAIT_TIMEOUT)
                    break;
                elapsed += interval;
                while (nextStat < _countof(statTimes) && elapsed >= statTimes[nextStat]) {
                    LogEarlySocketRedirectStats(
                        pi.hProcess, earlyBootstrapBase, earlyBootstrapData);
                    ++nextStat;
                }
            }
            if (survivalWait == WAIT_OBJECT_0) {
                DWORD exitCode = 0;
                if (GetExitCodeProcess(pi.hProcess, &exitCode))
                    printf("%ls process exited during observation: 0x%08lX\n",
                           EraName(era), exitCode);
                else
                    printf("%ls process exit observed; GetExitCodeProcess failed: %lu\n",
                           EraName(era), GetLastError());
            } else if (survivalWait == WAIT_TIMEOUT) {
                printf("%ls process remained active for the 15-second observation window\n",
                       EraName(era));
            } else {
                printf("%ls observation wait failed: %lu\n", EraName(era), GetLastError());
            }
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            if (file != nullptr)
                fclose(file);
            return 0;
        }

        // Bypass Hyperion
        HANDLE hBypass = OpenProcess(PROCESS_ALL_ACCESS, FALSE, targetPid);
        if (!hBypass) {
            printf("OpenProcess for bypass failed\n");
            TerminateProcess(pi.hProcess, 0xFFFFFFFF);
            WaitForSingleObject(pi.hProcess, 5000);
            CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
            return 1;
        }
        bool bypassOk = BypassHyperion(hBypass, targetPid, filePathStr.c_str());
        CloseHandle(hBypass);
        if (!bypassOk) {
            printf("Bypass failed\n");
            TerminateProcess(pi.hProcess, 0xFFFFFFFF);
            WaitForSingleObject(pi.hProcess, 5000);
            CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
            return 1;
        }

        // Second injection + manual map + external hook writing
        const wchar_t* injectPath = IsProcess64Bit(pi.hProcess) ? dllx64HyperionStr.c_str() : dllx86DirStr.c_str();
        printf("Injecting: %ls\n", injectPath);
        EngineLaunchResponse inject = Inject(targetPid, injectPath);
        if (inject != EngineLaunchResponse::Success) {
            printf("LoadLibraryW failed (error %d), manual mapping...\n", inject);
            auto dllData = ReadDllFile(injectPath);
            if (!dllData.empty()) {
                uintptr_t base = ManualMapDll(pi.hProcess, dllData);
                if (base) {
                    printf("Manual map succeeded at 0x%llX\n", (unsigned long long)base);
                    printf("Installing hooks from injector...\n");
                    // A five-byte inline jump is not atomic. Stop every target thread so none can
                    // execute a partially written prologue while the external hooks are installed.
                    SuspendProcess(targetPid);
                    InstallHooksFromInjector(pi.hProcess, base, dllData, era);
                    RestorePreparedIatProtection(pi.hProcess);
                    ResumeProcess(targetPid);
                    inject = EngineLaunchResponse::Success;
                } else { printf("Manual map failed\n"); }
            }
        }
        // No-op after the manual-map path; needed when LoadLibrary installed hooks directly.
        RestorePreparedIatProtection(pi.hProcess);
        if (inject != EngineLaunchResponse::Success) {
            printf("Failed to inject noobhook after all fallback methods: error %d\n", inject);
            TerminateProcess(pi.hProcess, 0xEEEEEEEE);
            WaitForSingleObject(pi.hProcess, 5000);
            CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
            return static_cast<int>(inject);
        }
        printf("Injection complete -- noobhook is running\n");

        const DWORD observation = WaitForSingleObject(pi.hProcess, 5000);
        if (observation == WAIT_OBJECT_0) {
            DWORD exitCode = 0;
            if (GetExitCodeProcess(pi.hProcess, &exitCode))
                printf("Player exited within observation window (status 0x%08lX)\n", exitCode);
            else
                printf("Player exited within observation window; GetExitCodeProcess failed (%lu)\n",
                       GetLastError());
        } else if (observation == WAIT_TIMEOUT) {
            printf("Player remained alive for the 5-second observation window\n");
        } else {
            printf("Player observation wait failed (%lu)\n", GetLastError());
        }

    } else {
#endif
        // ---- non-Hyperion processes ----
        DWORD creationFlags = CREATE_SUSPENDED;
        if (!createTargetProcess(creationFlags)) {
            printf("CreateProcessW failed: %lu (%s)\n", GetLastError(), LastErrorStr().c_str());
            return 7;
        }
        EngineLaunchResponse inject = Inject(GetProcessId(pi.hProcess), IsProcess64Bit(pi.hProcess) ? dllx64DirStr.c_str() : dllx86DirStr.c_str());
        if (inject != EngineLaunchResponse::Success) {
            printf("Failed to inject to target process: error %d\n", inject);
            TerminateProcess(pi.hProcess, 0xEEEEEEEE);
            WaitForSingleObject(pi.hProcess, 5000);
            CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
            return static_cast<int>(inject);
        }
        if (ResumeThread(pi.hThread) == -1) {
            printf("Can't resume thread of target process\n");
            TerminateProcess(pi.hProcess, 0xFFFFFFFF);
            WaitForSingleObject(pi.hProcess, 5000);
            CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
            return static_cast<int>(EngineLaunchResponse::InjectFailedToResumeProcess);
        }
#if defined(_M_X64)
    }
#endif

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    if (file != nullptr)
        fclose(file);
    return 0;
}
