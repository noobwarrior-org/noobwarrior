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
// Description: A separate program that injects into 32 bit processes, since 64-bit Windows processes cannot do this job.
#include <windows.h>

#include <string>
#include <vector>
#include <filesystem>
#include <cstdio>
#include <format>

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
    InjectFailedToResumeProcess
};

static std::string LastErrorStr(DWORD err = GetLastError()) {
    char buf[512] = {0};
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   buf, (DWORD)sizeof(buf), nullptr);
    return std::string(buf);
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
    if (hFile == INVALID_HANDLE_VALUE)
        return -1; // Error

    HANDLE hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (hMap == NULL) {
        CloseHandle(hFile);
        return -1;
    }

    LPVOID lpBase = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    if (lpBase == NULL) {
        CloseHandle(hMap);
        CloseHandle(hFile);
        return -1;
    }

    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)lpBase;
    if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        UnmapViewOfFile(lpBase);
        CloseHandle(hMap);
        CloseHandle(hFile);
        return -1;
    }

    PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((BYTE*)lpBase + pDosHeader->e_lfanew);
    if (pNtHeaders->Signature != IMAGE_NT_SIGNATURE) {
        UnmapViewOfFile(lpBase);
        CloseHandle(hMap);
        CloseHandle(hFile);
        return -1;
    }

    int bitness = 0;

    switch (pNtHeaders->FileHeader.Machine) {
    case IMAGE_FILE_MACHINE_I386:
        bitness = 32;
        break;
    case IMAGE_FILE_MACHINE_AMD64:
        bitness = 64;
        break;
    default:
        bitness = 0; // Unknown or other architecture
        break;
    }

    UnmapViewOfFile(lpBase);
    CloseHandle(hMap);
    CloseHandle(hFile);

    return bitness;
#else
    return -1;
#endif
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
        DWORD err = GetLastError();
        printf("VirtualAllocEx failed: %lu (%s)\n", err, LastErrorStr(err).c_str());
        CloseHandle(handle);
		return EngineLaunchResponse::InjectCannotWriteToProcessMemory;
    }

    if (!WriteProcessMemory(handle, mem, absPath, MAX_PATH, NULL)) {
        DWORD err = GetLastError();
        printf("WriteProcessMemory failed: %lu (%s)\n", err, LastErrorStr(err).c_str());
        res = EngineLaunchResponse::InjectCannotWriteToProcessMemory;
        goto cleanup;
    }

    HMODULE hKernel32;
    hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!hKernel32) {
        printf("Failed to get kernel32 handle\n");
        res = EngineLaunchResponse::InjectFailedToGetModuleHandle;
        goto cleanup;
    }

    FARPROC pLoadLibraryW;
    pLoadLibraryW = GetProcAddress(hKernel32, "LoadLibraryW");
    if (!pLoadLibraryW) {
        printf("Failed to get LoadLibraryW address\n");
        res = EngineLaunchResponse::InjectFailedToGetFunctionAddress;
        goto cleanup;
    }

    HANDLE thread;
    thread = CreateRemoteThread(handle, nullptr, 0, (LPTHREAD_START_ROUTINE)pLoadLibraryW, mem, 0, nullptr);
    if (thread == nullptr) {
        DWORD err = GetLastError();
        printf("CreateRemoteThread failed: %lu (%s)\n", err, LastErrorStr(err).c_str());
        res = EngineLaunchResponse::InjectCannotCreateThreadInProcess;
        goto cleanup;
    }
    if (WaitForSingleObject(thread, 30000) != WAIT_OBJECT_0) {
        DWORD err = GetLastError();
        printf("Thread wait failed or timed out: %lu (%s)\n", err, LastErrorStr(err).c_str());
        res = EngineLaunchResponse::InjectThreadTimedOut;
        goto cleanup;
    }

    GetExitCodeThread(thread, &exitCode);
    if (exitCode == 0) {
        printf("LoadLibraryW returned NULL - DLL failed to load\n");
        res = EngineLaunchResponse::InjectFailedToLoadLibrary;
        goto cleanup;
    }
    res = EngineLaunchResponse::Success;
cleanup:
    if (mem) VirtualFreeEx(handle, mem, 0, MEM_RELEASE);
    if (thread) CloseHandle(thread);
    CloseHandle(handle);
    return res;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int showCmd) {
    FILE* file = freopen("noobhook_injector.log", "w", stdout);

    int argc;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    if (argc == 1) {
        MessageBoxA(nullptr,
            R"(Note: This program is not meant to be used on its own. If you ran this without knowing what it does, just click OK and the program will close on its own.

Args:
--file: a path to the file you want to launch and inject a DLL to.
--ip: the IP address of the server to connect to.
--port: the port of the server to connect to.
--local: JSON data containing the player's name, user id, membership, and character appearance. Percent encoded. Only works on servers set to Local mode.)",
            "noobHook Injector",
            MB_ICONINFORMATION | MB_OK);
        return 0;
    }

    std::wstring filePathStr;
    std::wstring ipStr;
    std::wstring portStr;
    std::wstring localStr;
    std::wstring placeIdStr;
    for (int i = 0; i < argc; i++) {
        if (i + 1 >= argc)
            break;

        if (wcscmp(argv[i], L"--file") == 0) {
            filePathStr = argv[i + 1];

        }
        if (wcscmp(argv[i], L"--ip") == 0) {
            ipStr = argv[i + 1];
        }

        if (wcscmp(argv[i], L"--port") == 0) {
            portStr = argv[i + 1];
        }

        if (wcscmp(argv[i], L"--local") == 0) {
            localStr = argv[i + 1];
        }

        if (wcscmp(argv[i], L"--placeid") == 0) {
            placeIdStr = argv[i + 1];
        }
    }

    printf("File arg: %ls\nIp arg: %ls\nPort arg: %ls\nLocal arg: %ls\nPlaceId arg: %ls\n", filePathStr.c_str(), ipStr.c_str(), portStr.c_str(), localStr.c_str(), placeIdStr.c_str());

    std::wstring wargs;

    std::filesystem::path filePath = std::filesystem::path(filePathStr);
    if (filePathStr.empty()) {
        printf("No --file argument provided\n");
        return 1;
    }

    std::filesystem::path fileName = filePath.filename();
    std::filesystem::path fileDir = filePath.parent_path();
	std::wstring fileDirStr = fileDir.wstring();
	printf("Launching file %ls in working directory %ls\n", filePathStr.c_str(), fileDirStr.c_str());

    wargs += filePathStr;
    if (fileName.compare("RCCService.exe") == 0) {
        std::wstring rccPlaceId = placeIdStr.empty() ? L"1818" : placeIdStr;
        wargs += std::format(L" -console -verbose -placeid:{} -port 53641 -localtest \"gameserver.json\" -settingsfile \"DevSettingsFile.json\"", rccPlaceId);
    } else if (fileName.compare("RobloxPlayerBeta.exe") == 0) {
        wargs += std::format(L" -a \"http://www.roblox.com/Login/Negotiate.ashx\" -j \"http://www.roblox.com/Game/PlaceLauncher.ashx?ip={}&port={}&local={}&placeId={}\" -t \"1\"", ipStr, portStr, localStr, placeIdStr);
    }
    std::vector<wchar_t> wargs_vec(wargs.begin(), wargs.end());
    wargs_vec.push_back(L'\0');

    std::vector<wchar_t> filedir_vec(fileDirStr.begin(), fileDirStr.end());
    filedir_vec.push_back(L'\0');

    PROCESS_INFORMATION pi {};
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    if (!CreateProcessW(nullptr, wargs_vec.data(), nullptr, nullptr, FALSE, CREATE_SUSPENDED, nullptr, filedir_vec.data(), &si, &pi)) {
        DWORD err = GetLastError();
        printf("CreateProcessW failed: %lu (%s)\n", err, LastErrorStr(err).c_str());
        return 7;
    }
    EngineLaunchResponse inject = Inject(GetProcessId(pi.hProcess), IsProcess64Bit(pi.hProcess) ? L"./noobhook_x86-64.dll" : L"./noobhook_x86.dll");
    if (inject != EngineLaunchResponse::Success) {
        printf("Failed to inject to target process: error %d\n", inject);
        TerminateProcess(pi.hProcess, 0xEEEEEEEE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return static_cast<int>(inject);
    }
    if (ResumeThread(pi.hThread) == -1) {
        printf("Can't resume thread of target process\n");
        TerminateProcess(pi.hProcess, 0xFFFFFFFF);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return static_cast<int>(EngineLaunchResponse::InjectFailedToResumeProcess);
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    fclose(file);
    return 0;
}