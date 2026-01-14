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

static std::string LastErrorStr(DWORD err = GetLastError()) {
    char buf[512] = {0};
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   buf, (DWORD)sizeof(buf), nullptr);
    return std::string(buf);
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

static int Inject(unsigned long pid, const wchar_t *dllPath) {
    int res = 5;

    if (!std::filesystem::exists(dllPath)) {
        printf("DLL does not exist: %ls\n", std::filesystem::path(dllPath).c_str());
        return 6;
    }

    printf("Starting injection - PID: %lu, DLL: %ls\n", pid, std::filesystem::path(dllPath).c_str());
#if defined(_WIN32)
    HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (handle == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        printf("OpenProcess failed: %lu (%s)\n", err, LastErrorStr(err).c_str());
        return 7;
    }

    BOOL isWow64;
    IsWow64Process(handle, &isWow64);
    int dllBitness = GetDllBitness(dllPath);

    printf("Target WOW64: %s, DLL bitness: %i\n", isWow64 ? "yes" : "no", dllBitness);

    if (isWow64 && dllBitness != 32) {
        printf("Error, architecture mismatch\n");
        CloseHandle(handle);
        return 8;
    }

    SIZE_T size_bytes = (wcslen(dllPath) + 1) * sizeof(wchar_t);
    void *mem = VirtualAllocEx(handle, NULL, size_bytes, MEM_COMMIT, PAGE_READWRITE);
    if (!mem) {
        DWORD err = GetLastError();
        printf("VirtualAllocEx failed: %lu (%s)\n", err, LastErrorStr(err).c_str());
        CloseHandle(handle);
        return 9;
    }

    if (!WriteProcessMemory(handle, mem, dllPath, size_bytes, NULL)) {
        DWORD err = GetLastError();
        printf("WriteProcessMemory failed: %lu (%s)\n", err, LastErrorStr(err).c_str());
        res = 9;
        goto cleanup;
    }

    HMODULE hKernel32;
    hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!hKernel32) {
        printf("Failed to get kernel32 handle\n");
        res = 10;
        goto cleanup;
    }

    FARPROC pLoadLibraryW;
    pLoadLibraryW = GetProcAddress(hKernel32, "LoadLibraryW");
    if (!pLoadLibraryW) {
        printf("Failed to get LoadLibraryW address\n");
        res = 11;
        goto cleanup;
    }

    HANDLE thread;
    thread = CreateRemoteThread(handle, nullptr, 0, (LPTHREAD_START_ROUTINE)pLoadLibraryW, mem, 0, nullptr);
    if (thread == nullptr) {
        DWORD err = GetLastError();
        printf("CreateRemoteThread failed: %lu (%s)\n", err, LastErrorStr(err).c_str());
        res = 12;
        goto cleanup;
    }
    if (WaitForSingleObject(thread, 5000) != WAIT_OBJECT_0) {
        printf("Thread wait failed or timed out \n");
        res = 13;
        goto cleanup;
    }

    DWORD dwExitCode;
    if (!GetExitCodeThread(thread, &dwExitCode)) {
        DWORD err = GetLastError();
        printf("GetExitCodeThread failed: %lu (%s)\n", err, LastErrorStr(err).c_str());
        res = 14;
        goto cleanup;
    }

    if (dwExitCode == 0) {
        printf("LoadLibraryW returned NULL - injection failed\n");
        res = 15;
        goto cleanup;
    }

    res = 1;
cleanup:
    if (mem) VirtualFreeEx(handle, mem, 0, MEM_RELEASE);
    if (thread) CloseHandle(thread);
    CloseHandle(handle);
    return res;
#else
    return res;
#endif
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int showCmd) {
    int argc;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    if (argc == 1) {
        MessageBoxA(nullptr,
            "Note: This program is not meant to be used on its own. If you ran this without knowing what it does, just click OK and the program will close on its own.\n\nArgs:\n--file: a path to the file you want to launch and inject a DLL to.",
            "noobHook Injector",
            MB_ICONINFORMATION | MB_OK);
        return 0;
    }

    std::wstring filePath;

    for (int i = 0; i < argc; i++) {
        if (wcsncmp(argv[i], L"--file", sizeof("--file")) == 0) {
            if (i + 1 <= argc)
                filePath = argv[i + 1];
        }
    }

    std::wstring wargs = filePath + L" -console -verbose -placeid:1818 -port 53641";
    std::vector<wchar_t> wargs_vec(wargs.begin(), wargs.end());
    wargs_vec.push_back(L'\0');

    PROCESS_INFORMATION pi {};
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    if (!CreateProcessW(nullptr, wargs_vec.data(), nullptr, nullptr, FALSE, CREATE_SUSPENDED, nullptr, nullptr, &si, &pi)) {
        DWORD err = GetLastError();
        printf("CreateProcessW failed: %lu (%s)\n", err, LastErrorStr(err).c_str());
        return 7;
    }
    int inject = Inject(GetProcessId(pi.hProcess), L"./noobhook_x86.dll");
    if (!inject) {
        TerminateProcess(pi.hProcess, 0xEEEEEEEE);
        return inject;
    }
    if (ResumeThread(pi.hThread) == -1) {   
        TerminateProcess(pi.hProcess, 0xFFFFFFFF);
        CloseHandle(pi.hThread);
        return 16;
    }
    return 1;
}