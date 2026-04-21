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
// File: Hook.cpp
// Started by: Hattozo
// Started on: 3/16/2025
// Description: Contains main entrypoint for noobWarrior Roblox hook
#include "Hook.h"
#include "Patch/Patches.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#define SECURITY_WIN32
#include <sspi.h>

#include <MinHook.h>
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <strsafe.h>

using namespace NoobHook;

enum RobloxVersion {
    VER_UNKNOWN,
    VER_0_449_0_411458,
    VER_0_463_0_417004
};

enum CURLoption {
    CURLOPT_URL = 10000 + 2
};

void NoobHook::WriteMemory(uintptr_t address, const void* data, size_t size) {
    DWORD old_protection;
	VirtualProtect(reinterpret_cast<LPVOID>(address), size, PAGE_EXECUTE_READWRITE, &old_protection);
	memcpy(reinterpret_cast<void*>(address), data, size);
	VirtualProtect(reinterpret_cast<LPVOID>(address), size, old_protection, &old_protection);
}

DWORD StrLength(PCHAR str) {
    DWORD length = 0;
    while (str[length] != '\0') {
        length++;
    }
    return length;
}

char *GetProductVersion() {
    char exePathBuf[1024];
    GetModuleFileNameEx(GetCurrentProcess(), NULL, exePathBuf, sizeof(exePathBuf));

    DWORD handle = 0;
    DWORD fileVersionSize = GetFileVersionInfoSizeA(exePathBuf, &handle);

    LPBYTE buffer = nullptr;
    UINT bufferLength = 0;

    if (fileVersionSize == 0) {
        return (char*)'\0';
    }

    char *versionInfo = new char[fileVersionSize];
    if (!GetFileVersionInfoA(exePathBuf, handle, fileVersionSize, versionInfo))
        goto failed;
    
    if (VerQueryValueA(versionInfo, "\\StringFileInfo\\040904E4\\ProductVersion", (LPVOID*)&buffer, &bufferLength)) {
        delete[] versionInfo;
        return reinterpret_cast<char*>(buffer);
    }

    if (VerQueryValueA(versionInfo, "\\StringFileInfo\\000004B0\\ProductVersion", (LPVOID*)&buffer, &bufferLength)) {
        delete[] versionInfo;
        return reinterpret_cast<char*>(buffer);
    }
failed:
    delete[] versionInfo;
    return (char*)'\0';
}

RobloxVersion GetRobloxVersion() {
    char *ver = GetProductVersion();
    if (strncmp(ver, "0, 449, 0, 411458", strlen(ver)) == 0) {
        return VER_0_449_0_411458;
    } else if (strncmp(ver, "0, 463, 0, 417004", strlen(ver)) == 0) {
        return VER_0_463_0_417004;
    }
    return VER_UNKNOWN;
}

struct ProcWindow {
    unsigned long pid;
    HWND hwnd;
};

static BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam) {
    ProcWindow &data = *(ProcWindow*)lParam;
    unsigned long process_id = 0;
    GetWindowThreadProcessId(handle, &process_id);
    data.hwnd = handle;
    return TRUE;
}

static HWND GetWindow() {
    struct ProcWindow {
        unsigned long pid;
        HWND hwnd;
    } data;
    data.pid = GetCurrentProcessId();
    data.hwnd = NULL;
    EnumWindows(&EnumWindowsCallback, (LPARAM)&data);
    return data.hwnd;
}

// thank you Raymond Chen: https://devblogs.microsoft.com/oldnewthing/20060223-14/?p=32173
// And this guy: https://stackoverflow.com/a/16684288.
static void SuspendAllThreadsExceptMines(DWORD targetProcessId, DWORD targetThreadId) {
    HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (h != INVALID_HANDLE_VALUE)
    {
        THREADENTRY32 te;
        te.dwSize = sizeof(te);
        if (Thread32First(h, &te))
        {
            do
            {
                if (te.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(te.th32OwnerProcessID)) 
                {
                    // Suspend all threads EXCEPT the one we want to keep running
                    if (te.th32ThreadID != targetThreadId && te.th32OwnerProcessID == targetProcessId)
                    {
                        HANDLE thread = ::OpenThread(THREAD_ALL_ACCESS, FALSE, te.th32ThreadID);
                        if (thread != NULL)
                        {
                            SuspendThread(thread);
                            CloseHandle(thread);
                        }
                    }
                }
                te.dwSize = sizeof(te);
            } while (Thread32Next(h, &te));
        }
        CloseHandle(h);    
    }
}

static void ResumeAllThreadsExceptMines(DWORD targetProcessId, DWORD targetThreadId) {
    HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (h != INVALID_HANDLE_VALUE)
    {
        THREADENTRY32 te;
        te.dwSize = sizeof(te);
        if (Thread32First(h, &te))
        {
            do
            {
                if (te.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(te.th32OwnerProcessID)) 
                {
                    if (te.th32ThreadID != targetThreadId && te.th32OwnerProcessID == targetProcessId)
                    {
                        HANDLE thread = ::OpenThread(THREAD_ALL_ACCESS, FALSE, te.th32ThreadID);
                        if (thread != NULL)
                        {
                            ResumeThread(thread);
                            CloseHandle(thread);
                        }
                    }
                }
                te.dwSize = sizeof(te);
            } while (Thread32Next(h, &te));
        }
        CloseHandle(h);
    }
}

static int (WSAAPI* pOrigConnect)(SOCKET, const sockaddr*, int);
static int WSAAPI MyConnect(SOCKET s, const sockaddr* name, int namelen) {
    if (name != nullptr && namelen >= (int)sizeof(sockaddr_in) && name->sa_family == AF_INET) {
        sockaddr_in addrCopy = *(sockaddr_in*)name;
        int port = ntohs(addrCopy.sin_port);
        if (port == 80 || port == 443) { // check if its HTTP/HTTPS
            // if it is then redirect to our server emulator
            addrCopy.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
            addrCopy.sin_port = htons(port == 80 ? 8080 : 8081);
            return pOrigConnect(s, (sockaddr*)&addrCopy, namelen);
        }
    }
    return pOrigConnect(s, name, namelen);
}

DWORD WINAPI Thread(LPVOID param) {
    //SuspendAllThreadsExceptMines(GetCurrentProcessId(), GetCurrentThreadId());

    FILE* file = freopen("noobhook.log", "w", stdout);
    if (file == NULL) {
		MessageBoxA(NULL, "Failed to open log file for writing", "noobHook", MB_ICONERROR | MB_OK);
        TerminateProcess(GetCurrentProcess(), 0xFFFFFFFF);
        return -1;
    }
	printf("Initializing noobHook\n");

    printf("Initializing MinHook\n");
    MH_Initialize();
    MH_CreateHookApi(L"ws2_32", "connect", MyConnect, (LPVOID*)&pOrigConnect);
    MH_EnableHook(MH_ALL_HOOKS);

    printf("Patching...\n");
    Patches::RemoveTrustCheck(); // This should be commented out unless if you know what you're doing. It's not commented out though because I'm trying to debug something.
    Patches::RemoveSignatureCheck();
    Patches::RemoveTLSVerification();
    Patches::FixSettingsKeyMustBeDefined();

    printf("Done\n");
    fclose(file);

    //ResumeAllThreadsExceptMines(GetCurrentProcessId(), GetCurrentThreadId());
    return 0;
}

BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD reason, LPVOID lpReserved) {
    HANDLE hThread = NULL;
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);

        hThread = CreateThread(0, 0, Thread, hModule, CREATE_SUSPENDED, 0);
        SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);

        ResumeThread(hThread);
        CloseHandle(hThread);
        break;
    case DLL_PROCESS_DETACH:
        MH_Uninitialize();
        if (lpReserved != nullptr)
            break;
        break;
    default:
        break;
    }
    return TRUE;
}