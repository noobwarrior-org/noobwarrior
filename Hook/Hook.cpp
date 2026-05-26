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
#include "Ping.h"
#include "Patch/Patches.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <winhttp.h>

#define SECURITY_WIN32
#include <sspi.h>

#include <MinHook.h>
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <strsafe.h>

#include <atomic>

using namespace NoobHook;

FILE* NoobHook::gFile = nullptr;
uint16_t NoobHook::gEmuHttpsPort = 53640;
uint16_t NoobHook::gEmuHttpPort = 8080;

enum RobloxVersion {
    VER_UNKNOWN,
    VER_0_449_0_411458,
    VER_0_463_0_417004
};

void NoobHook::Out(const char* category, const char* format, ...) {
    if (gFile) {
        fprintf(gFile, "[NoobHook::%s] ", category);
        va_list args;
        va_start(args, format);
        vfprintf(gFile, format, args);
        fprintf(gFile, "\n");
        fflush(gFile);
        va_end(args);
    }
}

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

static char gVersionStringBuf[128] = {0};

char *GetProductVersion() {
    char exePathBuf[1024];
    GetModuleFileNameEx(GetCurrentProcess(), NULL, exePathBuf, sizeof(exePathBuf));

    DWORD handle = 0;
    DWORD fileVersionSize = GetFileVersionInfoSizeA(exePathBuf, &handle);
    if (fileVersionSize == 0)
        return nullptr;

    char *versionInfo = new char[fileVersionSize];
    LPBYTE buffer = nullptr;
    UINT bufferLength = 0;
    bool found = false;

    if (!GetFileVersionInfoA(exePathBuf, handle, fileVersionSize, versionInfo))
        goto cleanup;

    if (VerQueryValueA(versionInfo, "\\StringFileInfo\\040904E4\\ProductVersion", (LPVOID*)&buffer, &bufferLength) ||
        VerQueryValueA(versionInfo, "\\StringFileInfo\\000004B0\\ProductVersion", (LPVOID*)&buffer, &bufferLength)) {
        // Copy into a static buffer before freeing versionInfo — buffer points inside it.
        strncpy_s(gVersionStringBuf, sizeof(gVersionStringBuf), reinterpret_cast<char*>(buffer), _TRUNCATE);
        found = true;
    }

cleanup:
    delete[] versionInfo;
    return found ? gVersionStringBuf : nullptr;
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
    int sockType = 0;
    int optLen = sizeof(sockType);
    getsockopt(s, SOL_SOCKET, SO_TYPE, (char*)&sockType, &optLen);

    if (sockType == SOCK_STREAM && name->sa_family == AF_INET) { // check if its a TCP socket
        sockaddr_in addrCopy = *(sockaddr_in*)name;
        int port = ntohs(addrCopy.sin_port);
        if (port == 80 || port == 443) { // check if its HTTP/HTTPS
            // if it is then redirect to our server emulator
            addrCopy.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
            addrCopy.sin_port = htons(port == 80 ? gEmuHttpPort : gEmuHttpsPort);
            return pOrigConnect(s, (sockaddr*)&addrCopy, namelen);
        }
    }
    return pOrigConnect(s, name, namelen);
}

static HINTERNET (WINAPI* pOrigInternetConnectW)(HINTERNET, LPCWSTR, INTERNET_PORT, LPCWSTR, LPCWSTR, DWORD, DWORD, DWORD_PTR);
static HINTERNET WINAPI MyInternetConnectW(HINTERNET hInternet, LPCWSTR lpszServerName, INTERNET_PORT nServerPort, LPCWSTR lpszUserName, LPCWSTR lpszPassword, DWORD dwService, DWORD dwFlags, DWORD_PTR dwContext) {
    Out("InternetConnectW", "InternetConnectW to %ws:%d\n", lpszServerName, nServerPort);

    if (nServerPort == 80 || nServerPort == 443) {
        return pOrigInternetConnectW(hInternet, L"127.0.0.1",
            nServerPort == 80 ? gEmuHttpPort : gEmuHttpsPort,
            lpszUserName, lpszPassword, dwService, dwFlags, dwContext);
    }
    return pOrigInternetConnectW(hInternet, lpszServerName, nServerPort, lpszUserName, lpszPassword, dwService, dwFlags, dwContext);
}

static HINTERNET (WINAPI* pOrigWinHttpConnect)(HINTERNET, LPCWSTR, INTERNET_PORT, DWORD);
static HINTERNET WINAPI MyWinHttpConnect(HINTERNET hSession, LPCWSTR pswzServerName, INTERNET_PORT nServerPort, DWORD dwReserved) {
    Out("WinHttpConnect", "WinHttpConnect to %ws:%d", pswzServerName, nServerPort);

    if (nServerPort == 80 || nServerPort == 443) {
        return pOrigWinHttpConnect(hSession, L"127.0.0.1",
            nServerPort == 80 ? gEmuHttpPort : gEmuHttpsPort, dwReserved);
    }
    return pOrigWinHttpConnect(hSession, pswzServerName, nServerPort, dwReserved);
}

static BOOL (WINAPI* pOrigWinHttpSendRequest)(HINTERNET, LPCWSTR, DWORD, LPVOID, DWORD, DWORD, DWORD_PTR);
static BOOL WINAPI MyWinHttpSendRequest(HINTERNET hRequest, LPCWSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength, DWORD dwTotalLength, DWORD_PTR dwContext) {
    DWORD secFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA
                   | SECURITY_FLAG_IGNORE_CERT_CN_INVALID
                   | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID
                   | SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &secFlags, sizeof(secFlags));
    return pOrigWinHttpSendRequest(hRequest, lpszHeaders, dwHeadersLength, lpOptional, dwOptionalLength, dwTotalLength, dwContext);
}

static std::atomic<bool> gGoodbyeSent { false };
static void EmitGoodbyeOnce() {
    bool expected = false;
    if (!gGoodbyeSent.compare_exchange_strong(expected, true))
        return;
    NoobHook::SendGoodbye(static_cast<int>(GetCurrentProcessId()));
    Out("Exit", "Sent Goodbye ping");
}

static void (WINAPI* pOrigExitProcess)(UINT);
static void WINAPI MyExitProcess(UINT exitCode) {
    EmitGoodbyeOnce();
    pOrigExitProcess(exitCode);
}

static void (WINAPI* pOrigRtlExitUserProcess)(NTSTATUS);
static void WINAPI MyRtlExitUserProcess(NTSTATUS exitStatus) {
    EmitGoodbyeOnce();
    pOrigRtlExitUserProcess(exitStatus);
}

static NoobHook::ProcessInfo gProcessInfo;

static DWORD WINAPI HeartbeatThread(LPVOID) {
    while (!gGoodbyeSent.load()) {
        Sleep(5000);
        if (gGoodbyeSent.load()) break;
        NoobHook::SendHeartbeat(gProcessInfo);
    }
    return 0;
}

DWORD WINAPI Thread(LPVOID param) {
	Out("Main", "Initializing noobHook");

    char portBuf[16];
    if (GetEnvironmentVariableA("NOOBHOOK_HTTP_PORT", portBuf, sizeof(portBuf)) > 0)
        gEmuHttpPort = static_cast<uint16_t>(atoi(portBuf));
    if (GetEnvironmentVariableA("NOOBHOOK_HTTPS_PORT", portBuf, sizeof(portBuf)) > 0)
        gEmuHttpsPort = static_cast<uint16_t>(atoi(portBuf));
    Out("Main", "Emulator ports: HTTP=%d HTTPS=%d", gEmuHttpPort, gEmuHttpsPort);

    gProcessInfo = NoobHook::CollectProcessInfo();
    Out("Main", "Process info: pid=%d side=%d version=%s port=%d placeId=%lld",
        gProcessInfo.Pid, (int)gProcessInfo.Side, gProcessInfo.Version ? gProcessInfo.Version : "",
        gProcessInfo.Port, (long long)gProcessInfo.PlaceId);
    if (NoobHook::SendHello(gProcessInfo))
        Out("Main", "Sent Hello ping to server emulator");
    else
        Out("Main", "Failed to send Hello ping (server emulator unreachable?)");

    HANDLE hbThread = CreateThread(0, 0, HeartbeatThread, nullptr, 0, nullptr);
    if (hbThread) CloseHandle(hbThread);

    Out("Main", "Initializing MinHook");
    MH_Initialize();
    MH_CreateHookApi(L"ws2_32", "connect", MyConnect, (LPVOID*)&pOrigConnect);
    MH_CreateHookApi(L"wininet", "InternetConnectW", MyInternetConnectW, (LPVOID*)&pOrigInternetConnectW);
    MH_CreateHookApi(L"winhttp", "WinHttpConnect", MyWinHttpConnect, (LPVOID*)&pOrigWinHttpConnect);
    MH_CreateHookApi(L"winhttp", "WinHttpSendRequest", MyWinHttpSendRequest, (LPVOID*)&pOrigWinHttpSendRequest);
    MH_CreateHookApi(L"kernel32", "ExitProcess", MyExitProcess, (LPVOID*)&pOrigExitProcess);
    MH_CreateHookApi(L"ntdll", "RtlExitUserProcess", MyRtlExitUserProcess, (LPVOID*)&pOrigRtlExitUserProcess);
    MH_EnableHook(MH_ALL_HOOKS);
    
#if defined(_WIN64)
    char exePath[MAX_PATH] = {0};
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    const char* exeName = strrchr(exePath, '\\');
    exeName = exeName ? exeName + 1 : exePath;
    if (_stricmp(exeName, "RobloxStudioBeta.exe") == 0) {
        char dllDir[MAX_PATH] = {0};
        DWORD n = GetModuleFileNameA(reinterpret_cast<HMODULE>(param), dllDir, MAX_PATH);
        if (n > 0) {
            char* slash = strrchr(dllDir, '\\');
            if (slash) *slash = '\0';
            std::string path = std::string(dllDir) + "\\noobhook_localrcc.dll";
            Out("Main", "Studio binary detected -- loading %s", path.c_str());
            if (LoadLibraryA(path.c_str()) == nullptr)
                Out("Main", "Failed to load noobhook_localrcc.dll (err=%lu)", GetLastError());
        }
    }
#endif

    Out("Main", "Done");
    //fclose(file);

    return 0;
}

BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD reason, LPVOID lpReserved) {
    HANDLE hThread = NULL;
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        gFile = freopen("noobhook.log", "w", stdout);
        if (gFile == nullptr) {
            MessageBoxA(NULL, "Failed to open log file for writing.", "noobHook", MB_ICONWARNING | MB_OK);
        }

        Out("DllMain", "Applying patches...");
        Patches::RemoveTrustCheck(); // This should be commented out unless if you know what you're doing. It's not commented out though because I'm trying to debug something.
        Patches::RemoveSignatureCheck();
        Patches::RemoveTLSVerification();
        Patches::FixSettingsKeyMustBeDefined();
        Patches::FixInsertObjects();
        Patches::FixStudioUnableToConnect();

        DisableThreadLibraryCalls(hModule);

        hThread = CreateThread(0, 0, Thread, hModule, CREATE_SUSPENDED, 0);
        SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);

        ResumeThread(hThread);
        CloseHandle(hThread);
        break;
    case DLL_PROCESS_DETACH:
        EmitGoodbyeOnce();
        MH_Uninitialize();
        if (lpReserved != nullptr)
            break;
        break;
    default:
        break;
    }
    return TRUE;
}