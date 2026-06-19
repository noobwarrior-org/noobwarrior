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
#include "ScriptExecutor/ScriptExecutor.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <winhttp.h>

#define SECURITY_WIN32
#include <sspi.h>

#include <MinHook.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <strsafe.h>

#include <atomic>
#include <string>
#include <format>

using namespace NoobHook;

FILE* NoobHook::gFile = nullptr;
uint16_t NoobHook::gEmuHttpsPort = 53640;
uint16_t NoobHook::gEmuHttpPort = 8080;

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

enum RobloxVersion {
    VER_UNKNOWN,
    VER_0_449_0_411458,
    VER_0_463_0_417004
};

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

static bool RedirectConnectAddr(const char* api, SOCKET s, const sockaddr* name, sockaddr_in* out) {
    if (name == nullptr || name->sa_family != AF_INET)
        return false;

    int sockType = 0;
    int optLen = sizeof(sockType);
    getsockopt(s, SOL_SOCKET, SO_TYPE, (char*)&sockType, &optLen);
    if (sockType != SOCK_STREAM)
        return false;

    const sockaddr_in* in = reinterpret_cast<const sockaddr_in*>(name);
    int port = ntohs(in->sin_port);

    char origIp[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET, (void*)&in->sin_addr, origIp, sizeof(origIp));

    if (port != 80 && port != 443) {
        Out(api, "passthrough %s:%d (not HTTP/HTTPS)", origIp, port);
        return false;
    }

    *out = *in;
    out->sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    out->sin_port = htons(port == 80 ? gEmuHttpPort : gEmuHttpsPort);
    Out(api, "redirect %s:%d -> 127.0.0.1:%d", origIp, port, ntohs(out->sin_port));
    return true;
}

static int (WSAAPI* pOrigConnect)(SOCKET, const sockaddr*, int);
static int WSAAPI MyConnect(SOCKET s, const sockaddr* name, int namelen) {
    sockaddr_in redirected;
    if (RedirectConnectAddr("connect", s, name, &redirected))
        return pOrigConnect(s, (sockaddr*)&redirected, sizeof(redirected));
    return pOrigConnect(s, name, namelen);
}

static int (WSAAPI* pOrigWSAConnect)(SOCKET, const sockaddr*, int, LPWSABUF, LPWSABUF, LPQOS, LPQOS);
static int WSAAPI MyWSAConnect(SOCKET s, const sockaddr* name, int namelen,
                               LPWSABUF callerData, LPWSABUF calleeData, LPQOS sqos, LPQOS gqos) {
    sockaddr_in redirected;
    if (RedirectConnectAddr("WSAConnect", s, name, &redirected))
        return pOrigWSAConnect(s, (sockaddr*)&redirected, sizeof(redirected), callerData, calleeData, sqos, gqos);
    return pOrigWSAConnect(s, name, namelen, callerData, calleeData, sqos, gqos);
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

static wchar_t gMergedCaBundle[MAX_PATH] = {0};
static std::atomic<bool> gMergedCaReady { false };
static CRITICAL_SECTION gCaLock;

static HANDLE (WINAPI* pOrigCreateFileW)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
static HANDLE (WINAPI* pOrigCreateFileA)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);

static bool PathEndsWith(const wchar_t* path, const wchar_t* suffix) {
    if (!path) return false;
    size_t pl = wcslen(path), sl = wcslen(suffix);
    return pl >= sl && _wcsicmp(path + (pl - sl), suffix) == 0;
}

// Read a whole file via the original CreateFileW so we never re-enter our own hook.
static bool ReadWholeFileOrig(LPCWSTR path, std::string& out) {
    HANDLE h = pOrigCreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return false;
    out.clear();
    char buf[8192];
    DWORD read = 0;
    while (ReadFile(h, buf, sizeof(buf), &read, nullptr) && read > 0)
        out.append(buf, read);
    CloseHandle(h);
    return true;
}

// Build the merged bundle once. originalCacert is whatever path the engine asked for.
static bool BuildMergedCaBundle(LPCWSTR originalCacert) {
    wchar_t emuCert[MAX_PATH] = {0};
    if (GetEnvironmentVariableW(L"NOOBHOOK_EMU_CERT", emuCert, MAX_PATH) == 0) {
        Out("CaBundle", "NOOBHOOK_EMU_CERT not set -- leaving cacert.pem untouched");
        return false;
    }

    std::string original, emu;
    ReadWholeFileOrig(originalCacert, original); // may be empty; we still want the emu CA
    if (!ReadWholeFileOrig(emuCert, emu) || emu.empty()) {
        Out("CaBundle", "Failed to read emulator cert at %ws", emuCert);
        return false;
    }

    std::string merged = original;
    if (!merged.empty() && merged.back() != '\n') merged += '\n';
    merged += emu;
    if (merged.back() != '\n') merged += '\n';

    wchar_t tempDir[MAX_PATH] = {0};
    GetTempPathW(MAX_PATH, tempDir);
    swprintf(gMergedCaBundle, MAX_PATH, L"%snoobhook_ca_%lu.pem", tempDir, GetCurrentProcessId());

    HANDLE h = pOrigCreateFileW(gMergedCaBundle, GENERIC_WRITE, 0, nullptr,
                                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        Out("CaBundle", "Failed to create merged bundle at %ws", gMergedCaBundle);
        gMergedCaBundle[0] = L'\0';
        return false;
    }
    DWORD written = 0;
    WriteFile(h, merged.data(), (DWORD)merged.size(), &written, nullptr);
    CloseHandle(h);
    Out("CaBundle", "Merged emulator CA into %ws (%zu bytes)", gMergedCaBundle, merged.size());
    return true;
}

// Returns the merged-bundle path to open instead, or nullptr to pass through.
static const wchar_t* ResolveCaRedirect(LPCWSTR requested) {
    if (!PathEndsWith(requested, L"cacert.pem"))
        return nullptr;
    if (gMergedCaBundle[0] && _wcsicmp(requested, gMergedCaBundle) == 0)
        return nullptr; // already our merged file

    EnterCriticalSection(&gCaLock);
    bool ready = gMergedCaReady.load();
    if (!ready) {
        ready = BuildMergedCaBundle(requested);
        gMergedCaReady.store(ready);
    }
    LeaveCriticalSection(&gCaLock);
    return ready ? gMergedCaBundle : nullptr;
}

static HANDLE WINAPI MyCreateFileW(LPCWSTR lpFileName, DWORD access, DWORD share,
                                   LPSECURITY_ATTRIBUTES sec, DWORD disp, DWORD flags, HANDLE tmpl) {
    if (const wchar_t* redir = ResolveCaRedirect(lpFileName)) {
        Out("CreateFileW", "redirect %ws -> %ws", lpFileName, redir);
        return pOrigCreateFileW(redir, access, share, sec, disp, flags, tmpl);
    }
    return pOrigCreateFileW(lpFileName, access, share, sec, disp, flags, tmpl);
}

static HANDLE WINAPI MyCreateFileA(LPCSTR lpFileName, DWORD access, DWORD share,
                                   LPSECURITY_ATTRIBUTES sec, DWORD disp, DWORD flags, HANDLE tmpl) {
    if (lpFileName) {
        wchar_t wide[MAX_PATH] = {0};
        if (MultiByteToWideChar(CP_ACP, 0, lpFileName, -1, wide, MAX_PATH) > 0) {
            if (const wchar_t* redir = ResolveCaRedirect(wide)) {
                Out("CreateFileA", "redirect %s -> %ws", lpFileName, redir);
                return pOrigCreateFileW(redir, access, share, sec, disp, flags, tmpl);
            }
        }
    }
    return pOrigCreateFileA(lpFileName, access, share, sec, disp, flags, tmpl);
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
    Patches::InstallCrashDiagnostics_LogBacktrace();
    EmitGoodbyeOnce();
    pOrigExitProcess(exitCode);
}

static void (WINAPI* pOrigRtlExitUserProcess)(NTSTATUS);
static void WINAPI MyRtlExitUserProcess(NTSTATUS exitStatus) {
    Patches::InstallCrashDiagnostics_LogBacktrace();
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

    Patches::InstallCrashDiagnostics();

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
    InitializeCriticalSection(&gCaLock);
    MH_Initialize();
    auto hook = [](const wchar_t* mod, const char* fn, LPVOID detour, LPVOID* orig) {
        MH_STATUS st = MH_CreateHookApi(mod, fn, detour, orig);
        Out("MinHook", "CreateHookApi(%ls!%s) = %d (%s)", mod, fn, (int)st,
            st == MH_OK ? "ok" : "FAILED");
    };
    hook(L"ws2_32", "connect", MyConnect, (LPVOID*)&pOrigConnect);
    hook(L"ws2_32", "WSAConnect", MyWSAConnect, (LPVOID*)&pOrigWSAConnect);
    hook(L"wininet", "InternetConnectW", MyInternetConnectW, (LPVOID*)&pOrigInternetConnectW);
    hook(L"winhttp", "WinHttpConnect", MyWinHttpConnect, (LPVOID*)&pOrigWinHttpConnect);
    hook(L"winhttp", "WinHttpSendRequest", MyWinHttpSendRequest, (LPVOID*)&pOrigWinHttpSendRequest);
    hook(L"kernel32", "CreateFileW", MyCreateFileW, (LPVOID*)&pOrigCreateFileW);
    hook(L"kernel32", "CreateFileA", MyCreateFileA, (LPVOID*)&pOrigCreateFileA);
    hook(L"kernel32", "ExitProcess", MyExitProcess, (LPVOID*)&pOrigExitProcess);
    hook(L"ntdll", "RtlExitUserProcess", MyRtlExitUserProcess, (LPVOID*)&pOrigRtlExitUserProcess);
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

    ScriptExecutor::Install();

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

#if defined(_M_IX86)
        Patches::InstallClusterNullGuard(); // survive the player's corrupt-union cluster crashes
#endif
        Out("DllMain", "Applying patches...");
        Patches::RemoveTrustCheck(); // This should be commented out unless if you know what you're doing. It's not commented out though because I'm trying to debug something.
        Patches::RemoveSignatureCheck();
        Patches::RemoveTLSVerification();
        Patches::FixSettingsKeyMustBeDefined();
        Patches::FixInsertObjects();

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