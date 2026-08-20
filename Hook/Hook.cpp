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
#ifdef NOOBHOOK_HYPERION
#include <buffer.h>
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincrypt.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <strsafe.h>

#include <atomic>
#include <string>
#include <format>

using namespace NoobHook;

#ifdef NOOBHOOK_HYPERION
// Hook enable table — filled by Thread(), read by the injector
// Placed in a dedicated PE section so the injector can find it easily
#pragma section(".hkinfo", read, write)
__declspec(allocate(".hkinfo")) HookInfoBlock g_HookInfo = {
    0x4B4F4F484E4F4F42ULL, // magic "BOONHOOK"
    {}, // table (zero-initialized)
    0    // count
};

static void* g_HookTableMagicRef = (void*)&g_HookInfo.magic;
#endif

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

static bool RedirectConnectAddr(const char* api, SOCKET s, const sockaddr* name, sockaddr_in* out,
                                bool querySocketType = true) {

    if (name == nullptr || name->sa_family != AF_INET)
        return false;

    if (querySocketType) {
        int sockType = 0;
        int optLen = sizeof(sockType);
        getsockopt(s, SOL_SOCKET, SO_TYPE, reinterpret_cast<char*>(&sockType), &optLen);
        if (sockType != SOCK_STREAM)
            return false;
    }

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
static int (WSAAPI* pOrigWSAConnect)(SOCKET, const sockaddr*, int,
                                     LPWSABUF, LPWSABUF, LPQOS, LPQOS);
static int (WSAAPI* pOrigWSPConnect)(SOCKET, const sockaddr*, int,
                                     LPWSABUF, LPWSABUF, LPQOS, LPQOS, LPINT);

static int WSAAPI MyConnect(SOCKET s, const sockaddr* name, int namelen) {
    sockaddr_in redirected;
    if (RedirectConnectAddr("connect", s, name, &redirected)) {
#ifdef NOOBHOOK_HYPERION
        // The injector detours WS2_32!connect itself on 0.728. Calling that export here would
        // recurse, so use the equivalent untouched WSAConnect entry point as the gateway.
        const int result = pOrigWSAConnect(s, reinterpret_cast<sockaddr*>(&redirected),
                                           sizeof(redirected), nullptr, nullptr, nullptr, nullptr);
#else
        const int result = pOrigConnect(s, (sockaddr*)&redirected, sizeof(redirected));
#endif
        if (result != 0) {
            const int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK)
                Out("connect", "redirected connect to 127.0.0.1:%d FAILED with WSA error %d",
                    ntohs(redirected.sin_port), error);
        }
        return result;
    }
#ifdef NOOBHOOK_HYPERION
    return pOrigWSAConnect(s, name, namelen, nullptr, nullptr, nullptr, nullptr);
#else
    return pOrigConnect(s, name, namelen);
#endif
}

static int WSAAPI MyWSAConnect(SOCKET s, const sockaddr* name, int namelen,
                               LPWSABUF callerData, LPWSABUF calleeData, LPQOS sqos, LPQOS gqos) {
    sockaddr_in redirected;
    if (RedirectConnectAddr("WSAConnect", s, name, &redirected)) {
        return pOrigWSAConnect(s, (sockaddr*)&redirected, sizeof(redirected), callerData, calleeData, sqos, gqos);
    }
    return pOrigWSAConnect(s, name, namelen, callerData, calleeData, sqos, gqos);
}

// WS2_32 dispatches connect through the provider's WSPPROC_TABLE.  On 0.728 the injector changes
// only the writable, heap-backed copy of that table, leaving both Roblox and Windows image pages
// pristine.  The extra errno argument is part of the Winsock SPI contract and must be forwarded.
static int WSAAPI MyWSPConnect(SOCKET s, const sockaddr* name, int namelen,
                               LPWSABUF callerData, LPWSABUF calleeData,
                               LPQOS sqos, LPQOS gqos, LPINT error) {
    sockaddr_in redirected;
    // This runs below WS2_32's socket lookup. Re-entering getsockopt for the same socket here can
    // contend with that lookup, so avoid the optional SO_TYPE query. Roblox's HTTP transports use
    // TCP for the only redirected destination ports; non-HTTP ports still pass through unchanged.
    if (RedirectConnectAddr("WSPConnect", s, name, &redirected, false)) {
        return pOrigWSPConnect(s, reinterpret_cast<sockaddr*>(&redirected), sizeof(redirected),
                               callerData, calleeData, sqos, gqos, error);
    }
    return pOrigWSPConnect(s, name, namelen, callerData, calleeData, sqos, gqos, error);
}

static bool IsEmulatedHost(const char* host) {
    if (host == nullptr || *host == 0)
        return false;
    static const char* kDomains[] = {"roblox.com", "rbxcdn.com", "robloxlabs.com", "rbxinfra.com"};
    const size_t hostLength = strlen(host);
    for (const char* domain : kDomains) {
        const size_t domainLength = strlen(domain);
        if (hostLength < domainLength)
            continue;
        if (_stricmp(host + (hostLength - domainLength), domain) != 0)
            continue;
        if (hostLength == domainLength || host[hostLength - domainLength - 1] == '.')
            return true;
    }
    return false;
}

static int (WSAAPI* pOrigGetAddrInfo)(PCSTR, PCSTR, const ADDRINFOA*, PADDRINFOA*);
static INT (WSAAPI* pOrigGetAddrInfoW)(PCWSTR, PCWSTR, const ADDRINFOW*, PADDRINFOW*);

static int WSAAPI MyGetAddrInfo(PCSTR node, PCSTR service, const ADDRINFOA* hints,
                                PADDRINFOA* result) {
    if (IsEmulatedHost(node)) {
        ADDRINFOA local {};
        if (hints != nullptr)
            local = *hints;
        local.ai_family = AF_INET;  // the emulator listens on IPv4 loopback only
        Out("getaddrinfo", "%s -> 127.0.0.1 (resolved locally)", node);
        return pOrigGetAddrInfo("127.0.0.1", service, &local, result);
    }
    return pOrigGetAddrInfo(node, service, hints, result);
}

static INT WSAAPI MyGetAddrInfoW(PCWSTR node, PCWSTR service, const ADDRINFOW* hints,
                                 PADDRINFOW* result) {
    char narrow[256] = {0};
    if (node != nullptr)
        WideCharToMultiByte(CP_UTF8, 0, node, -1, narrow, sizeof(narrow) - 1, nullptr, nullptr);
    if (IsEmulatedHost(narrow)) {
        ADDRINFOW local {};
        if (hints != nullptr)
            local = *hints;
        local.ai_family = AF_INET;
        Out("GetAddrInfoW", "%s -> 127.0.0.1 (resolved locally)", narrow);
        return pOrigGetAddrInfoW(L"127.0.0.1", service, &local, result);
    }
    return pOrigGetAddrInfoW(node, service, hints, result);
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

static BOOL (WINAPI* pOrigCertVerifyChainPolicy)(LPCSTR, PCCERT_CHAIN_CONTEXT, PCERT_CHAIN_POLICY_PARA, PCERT_CHAIN_POLICY_STATUS);
static BOOL WINAPI MyCertVerifyCertificateChainPolicy(LPCSTR pszPolicyOID, PCCERT_CHAIN_CONTEXT pChainContext,
                                                      PCERT_CHAIN_POLICY_PARA pPolicyPara,
                                                      PCERT_CHAIN_POLICY_STATUS pPolicyStatus) {
    BOOL r = pOrigCertVerifyChainPolicy(pszPolicyOID, pChainContext, pPolicyPara, pPolicyStatus);
    DWORD wasError = pPolicyStatus ? pPolicyStatus->dwError : 0;
    if (pPolicyStatus) pPolicyStatus->dwError = 0; // ERROR_SUCCESS == trusted
    Out("CertVerify", "CertVerifyCertificateChainPolicy oid=%Iu ret=%d dwError=0x%lx -> forced success",
        (size_t)(ULONG_PTR)pszPolicyOID, r, (unsigned long)wasError);
    return TRUE;
}

static BOOL (WINAPI* pOrigCertGetCertificateChain)(HCERTCHAINENGINE, PCCERT_CONTEXT, LPFILETIME, HCERTSTORE,
                                                   PCERT_CHAIN_PARA, DWORD, LPVOID, PCCERT_CHAIN_CONTEXT*);
static BOOL WINAPI MyCertGetCertificateChain(HCERTCHAINENGINE hChainEngine, PCCERT_CONTEXT pCertContext, LPFILETIME pTime,
                                             HCERTSTORE hAdditionalStore, PCERT_CHAIN_PARA pChainPara, DWORD dwFlags,
                                             LPVOID pvReserved, PCCERT_CHAIN_CONTEXT* ppChainContext) {
    static int n = 0;
    BOOL r = pOrigCertGetCertificateChain(hChainEngine, pCertContext, pTime, hAdditionalStore,
                                          pChainPara, dwFlags, pvReserved, ppChainContext);
    if (n < 20) { n++; Out("CertVerify", "CertGetCertificateChain ret=%d (schannel cert path active)", r); }
    return r;
}

static wchar_t gMergedCaBundle[MAX_PATH] = {0};
static std::atomic<bool> gMergedCaReady { false };
static CRITICAL_SECTION gCaLock;
// Identify the thread doing the merge so its own ntdll file operations do not recurse. A global bool
// would also let unrelated threads bypass the redirect; thread_local is unavailable because the
// Hyperion DLL is manually mapped and has no loader-initialized static TLS slot.
static std::atomic<DWORD> gCaResolverThreadId { 0 };

static HANDLE (WINAPI* pOrigCreateFileW)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
static HANDLE (WINAPI* pOrigCreateFileA)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);

static bool PathEndsWith(const wchar_t* path, const wchar_t* suffix) {
    if (!path) return false;
    size_t pl = wcslen(path), sl = wcslen(suffix);
    return pl >= sl && _wcsicmp(path + (pl - sl), suffix) == 0;
}

// Read a whole file via the original CreateFileW so we never re-enter our own hook.
static bool ReadWholeFileOrig(LPCWSTR path, std::string& out) {
    if (!pOrigCreateFileW)
        return false;
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
    bool writeOk = WriteFile(h, merged.data(), static_cast<DWORD>(merged.size()), &written, nullptr)
        && written == merged.size();
    CloseHandle(h);
    if (!writeOk) {
        DeleteFileW(gMergedCaBundle);
        Out("CaBundle", "Failed to write complete merged bundle at %ws", gMergedCaBundle);
        gMergedCaBundle[0] = L'\0';
        return false;
    }
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
        gCaResolverThreadId.store(GetCurrentThreadId(), std::memory_order_release);
        ready = BuildMergedCaBundle(requested);
        gCaResolverThreadId.store(0, std::memory_order_release);
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

// Studio 0.729 opens ssl/cacert.pem below kernel32 (its statically-linked OpenSSL/curl calls straight
// into ntdll), so MyCreateFileW/A never see it and the CA merge never happens -- the self-signed
// emulator cert is then rejected and the splash hangs. Hook the syscall stubs that every file open
// funnels through and apply the same redirect there. This is API-independent, so it also covers older
// builds without changing their behaviour (their kernel32 opens still get redirected, just one layer
// deeper), and it keeps the engine's own ssl/cacert.pem untouched on disk.
typedef NTSTATUS (NTAPI* NtCreateFile_t)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PVOID /*PIO_STATUS_BLOCK*/,
                                         PLARGE_INTEGER, ULONG, ULONG, ULONG, ULONG, PVOID, ULONG);
typedef NTSTATUS (NTAPI* NtOpenFile_t)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PVOID /*PIO_STATUS_BLOCK*/,
                                       ULONG, ULONG);
static NtCreateFile_t pOrigNtCreateFile = nullptr;
static NtOpenFile_t   pOrigNtOpenFile   = nullptr;

// If this open targets a cacert.pem, resolve the merged bundle and fill an OBJECT_ATTRIBUTES pointing
// at it (NT path form "\??\<win32 path>"); returns false to pass the open through untouched. The
// caller owns ntBuf/usOut/oaOut, which must outlive the following NtCreateFile/NtOpenFile call.
static bool CaRedirectNt(POBJECT_ATTRIBUTES in, wchar_t* ntBuf, size_t ntBufCch,
                         UNICODE_STRING* usOut, OBJECT_ATTRIBUTES* oaOut) {
    if (gCaResolverThreadId.load(std::memory_order_acquire) == GetCurrentThreadId())
        return false;                               // a read/write issued by the merge itself
    if (!in || !in->ObjectName || !in->ObjectName->Buffer) return false;
    const UNICODE_STRING* nm = in->ObjectName;
    size_t chars = nm->Length / sizeof(wchar_t);
    if (chars < 10 || _wcsnicmp(nm->Buffer + (chars - 10), L"cacert.pem", 10) != 0)
        return false;                               // fast reject: not a cacert.pem open

    // Null-terminated copy for the merge read; drop the NT "\??\" device prefix so it's a plain
    // Win32 path CreateFileW can open.
    wchar_t full[MAX_PATH];
    size_t cc = chars < MAX_PATH ? chars : MAX_PATH - 1;
    wmemcpy(full, nm->Buffer, cc);
    full[cc] = L'\0';
    const wchar_t* win32 = full;
    if (wcsncmp(win32, L"\\??\\", 4) == 0) win32 += 4;

    const wchar_t* merged = ResolveCaRedirect(win32);
    if (!merged) return false;

    swprintf(ntBuf, ntBufCch, L"\\??\\%s", merged);
    usOut->Buffer = ntBuf;
    usOut->Length = (USHORT)(wcslen(ntBuf) * sizeof(wchar_t));
    usOut->MaximumLength = (USHORT)(usOut->Length + sizeof(wchar_t));
    *oaOut = *in;                                   // keep Attributes/SecurityDescriptor/RootDirectory flags
    oaOut->ObjectName = usOut;
    oaOut->RootDirectory = nullptr;                 // ntBuf is an absolute \??\ path
    Out("NtFile", "redirect %ws -> %ws", full, ntBuf);
    return true;
}

static NTSTATUS NTAPI MyNtCreateFile(PHANDLE FileHandle, ACCESS_MASK Access, POBJECT_ATTRIBUTES ObjectAttributes,
                                     PVOID /*PIO_STATUS_BLOCK*/ IoStatusBlock, PLARGE_INTEGER AllocationSize,
                                     ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition,
                                     ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength) {
    wchar_t ntBuf[MAX_PATH + 8];
    UNICODE_STRING us;
    OBJECT_ATTRIBUTES oa;
    if (CaRedirectNt(ObjectAttributes, ntBuf, _countof(ntBuf), &us, &oa))
        return pOrigNtCreateFile(FileHandle, Access, &oa, IoStatusBlock, AllocationSize, FileAttributes,
                                 ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
    return pOrigNtCreateFile(FileHandle, Access, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes,
                             ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
}

static NTSTATUS NTAPI MyNtOpenFile(PHANDLE FileHandle, ACCESS_MASK Access, POBJECT_ATTRIBUTES ObjectAttributes,
                                   PVOID /*PIO_STATUS_BLOCK*/ IoStatusBlock, ULONG ShareAccess, ULONG OpenOptions) {
    wchar_t ntBuf[MAX_PATH + 8];
    UNICODE_STRING us;
    OBJECT_ATTRIBUTES oa;
    if (CaRedirectNt(ObjectAttributes, ntBuf, _countof(ntBuf), &us, &oa))
        return pOrigNtOpenFile(FileHandle, Access, &oa, IoStatusBlock, ShareAccess, OpenOptions);
    return pOrigNtOpenFile(FileHandle, Access, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
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

#ifdef NOOBHOOK_HYPERION
static FARPROC (WINAPI* pOrigGetProcAddress)(HMODULE, LPCSTR);

static FARPROC WINAPI MyGetProcAddress(HMODULE module, LPCSTR procName) {
    FARPROC resolved = pOrigGetProcAddress(module, procName);
    if (resolved == nullptr || procName == nullptr || IS_INTRESOURCE(procName))
        return resolved;

    // Roblox 0.728's embedded HTTP stack obtains Winsock entry points at runtime instead of
    // keeping them in a conventional PE import table.  The external injector redirects this
    // GetProcAddress import, then this detour substitutes only exports whose resolved address is
    // exactly the original API.  Comparing the address as well as the name avoids changing an
    // unrelated module that happens to export a function with the same name.
    #define REDIRECT_RESOLVED_API(name, original, detour) \
        if (strcmp(procName, name) == 0 && resolved == reinterpret_cast<FARPROC>(original)) { \
            Out("GetProcAddress", "redirect dynamically resolved %s", name); \
            return reinterpret_cast<FARPROC>(detour); \
        }

    REDIRECT_RESOLVED_API("connect", pOrigConnect, MyConnect);
    REDIRECT_RESOLVED_API("WSAConnect", pOrigWSAConnect, MyWSAConnect);
    REDIRECT_RESOLVED_API("WinHttpConnect", pOrigWinHttpConnect, MyWinHttpConnect);
    REDIRECT_RESOLVED_API("WinHttpSendRequest", pOrigWinHttpSendRequest, MyWinHttpSendRequest);
    REDIRECT_RESOLVED_API("CreateFileW", pOrigCreateFileW, MyCreateFileW);
    REDIRECT_RESOLVED_API("CreateFileA", pOrigCreateFileA, MyCreateFileA);
    REDIRECT_RESOLVED_API("NtCreateFile", pOrigNtCreateFile, MyNtCreateFile);
    REDIRECT_RESOLVED_API("NtOpenFile", pOrigNtOpenFile, MyNtOpenFile);
    REDIRECT_RESOLVED_API("ExitProcess", pOrigExitProcess, MyExitProcess);
    REDIRECT_RESOLVED_API("RtlExitUserProcess", pOrigRtlExitUserProcess, MyRtlExitUserProcess);
    REDIRECT_RESOLVED_API("CertVerifyCertificateChainPolicy", pOrigCertVerifyChainPolicy,
                          MyCertVerifyCertificateChainPolicy);
    REDIRECT_RESOLVED_API("CertGetCertificateChain", pOrigCertGetCertificateChain,
                          MyCertGetCertificateChain);

    #undef REDIRECT_RESOLVED_API
    return resolved;
}
#endif

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
    
    {
        wchar_t wv2Args[512];
        swprintf(wv2Args, 512,
            L"--host-resolver-rules=\"MAP *.roblox.com 127.0.0.1:%u,MAP roblox.com 127.0.0.1:%u\" --ignore-certificate-errors",
            (unsigned)gEmuHttpsPort, (unsigned)gEmuHttpsPort);
        SetEnvironmentVariableW(L"WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS", wv2Args);
        Out("Main", "Routed WebView2 -> emulator :%d via WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS", gEmuHttpsPort);
    }

    gProcessInfo = NoobHook::CollectProcessInfo();
    Out("Main", "Process info: pid=%d side=%d version=%s port=%d placeId=%lld",
        gProcessInfo.Pid, (int)gProcessInfo.Side, gProcessInfo.Version ? gProcessInfo.Version : "",
        gProcessInfo.Port, (long long)gProcessInfo.PlaceId);
    if (NoobHook::SendHello(gProcessInfo))
        Out("Main", "Sent Hello ping to server emulator");
    else
        Out("Main", "Failed to send Hello ping (server emulator unreachable?)");

#ifndef NOOBHOOK_HYPERION
    HANDLE hbThread = CreateThread(0, 0, HeartbeatThread, nullptr, 0, nullptr);
    if (hbThread) CloseHandle(hbThread);
#else
    // Do not perform HTTP work from a Winsock detour. Hyperion enters parts of its transport
    // path with private stack state, and a synchronous heartbeat here re-enters that path.
    Out("Main", "Hyperion socket detours use no re-entrant heartbeat");
#endif

    Out("Main", "Initializing MinHook");
    InitializeCriticalSection(&gCaLock);
    MH_Initialize();
    auto hook = [](const wchar_t* mod, const char* fn, LPVOID detour, LPVOID* orig) -> bool {
        Out("MinHook", "CreateHookApi(%ls!%s) START", mod, fn);
        MH_STATUS st = MH_CreateHookApi(mod, fn, detour, orig);
        Out("MinHook", "CreateHookApi(%ls!%s) = %d (%s)", mod, fn, (int)st,
            st == MH_OK ? "ok" : "FAILED");
#ifdef NOOBHOOK_HYPERION
        if (st != MH_OK) {
            // The clean 0.728 process policy prevents MinHook from allocating
            // executable trampolines.  External IAT redirection does not need
            // a trampoline: the detour can call the untouched export directly.
            HMODULE module = GetModuleHandleW(mod);
            FARPROC target = module ? GetProcAddress(module, fn) : nullptr;
            if (target && orig) {
                *orig = reinterpret_cast<LPVOID>(target);
                Out("MinHook", "Using direct-export original for external IAT hook %ls!%s -> %p",
                    mod, fn, target);
                return true;
            }
        }
#endif
        return st == MH_OK;
    };
    Out("Main", "Installing hooks...");
#ifdef NOOBHOOK_HYPERION
    // ACG prevents MinHook from allocating a private executable trampoline. The 0.728 injector
    // redirects the writable Winsock provider table and publishes its original WSPConnect target;
    // older Hyperion paths can still consume the conventional connect entry below.
    HMODULE winsock = GetModuleHandleW(L"ws2_32.dll");
    pOrigConnect = winsock
        ? reinterpret_cast<decltype(pOrigConnect)>(GetProcAddress(winsock, "connect")) : nullptr;
    pOrigWSAConnect = winsock
        ? reinterpret_cast<decltype(pOrigWSAConnect)>(GetProcAddress(winsock, "WSAConnect")) : nullptr;
    bool madeConnect = pOrigConnect != nullptr && pOrigWSAConnect != nullptr;
    bool madeWSAConnect = false;
    bool madeWSPConnect = true;
    Out("Main", "Prepared external WS2_32!connect detour (connect=%p WSAConnect=%p)",
        pOrigConnect, pOrigWSAConnect);
#else
    bool madeConnect = hook(L"ws2_32", "connect", MyConnect, (LPVOID*)&pOrigConnect);
    bool madeWSAConnect = hook(L"ws2_32", "WSAConnect", MyWSAConnect, (LPVOID*)&pOrigWSAConnect);
    bool madeWSPConnect = false;
    hook(L"ws2_32", "getaddrinfo", MyGetAddrInfo, (LPVOID*)&pOrigGetAddrInfo);
    hook(L"ws2_32", "GetAddrInfoW", MyGetAddrInfoW, (LPVOID*)&pOrigGetAddrInfoW);
#endif
#ifdef NOOBHOOK_HYPERION
    // The 0.728 image-backed payload only needs callable socket detours. Every additional
    // MH_CreateHookApi/LoadLibrary operation lengthens synchronous DllMain execution, and the app
    // shell can exit before the injector receives the hook table. The on-disk CA merge handles TLS
    // trust for this path; the bootstrap already covers GetProcAddress.
    bool madeWinHttpConnect = false;
    bool madeWinHttpSendRequest = false;
    bool madeCreateFileW = false;
    bool madeCreateFileA = false;
    bool madeNtCreateFile = false;
    bool madeNtOpenFile = false;
    bool madeExitProcess = false;
    bool madeRtlExitUserProcess = false;
    bool madeCertVerify = false;
    bool madeCertGetChain = false;
    bool madeGetProcAddress = false;
#else
    hook(L"wininet", "InternetConnectW", MyInternetConnectW, (LPVOID*)&pOrigInternetConnectW);
    bool madeWinHttpConnect = hook(L"winhttp", "WinHttpConnect", MyWinHttpConnect, (LPVOID*)&pOrigWinHttpConnect);
    bool madeWinHttpSendRequest = hook(L"winhttp", "WinHttpSendRequest", MyWinHttpSendRequest, (LPVOID*)&pOrigWinHttpSendRequest);
    bool madeCreateFileW = hook(L"kernel32", "CreateFileW", MyCreateFileW, (LPVOID*)&pOrigCreateFileW);
    bool madeCreateFileA = hook(L"kernel32", "CreateFileA", MyCreateFileA, (LPVOID*)&pOrigCreateFileA);
    bool madeNtCreateFile = hook(L"ntdll", "NtCreateFile", MyNtCreateFile, (LPVOID*)&pOrigNtCreateFile);
    bool madeNtOpenFile = hook(L"ntdll", "NtOpenFile", MyNtOpenFile, (LPVOID*)&pOrigNtOpenFile);
    bool madeExitProcess = hook(L"kernel32", "ExitProcess", MyExitProcess, (LPVOID*)&pOrigExitProcess);
    bool madeRtlExitUserProcess = hook(L"ntdll", "RtlExitUserProcess", MyRtlExitUserProcess, (LPVOID*)&pOrigRtlExitUserProcess);
    LoadLibraryW(L"crypt32.dll");
    bool madeCertVerify = hook(L"crypt32", "CertVerifyCertificateChainPolicy", MyCertVerifyCertificateChainPolicy, (LPVOID*)&pOrigCertVerifyChainPolicy);
    bool madeCertGetChain = hook(L"crypt32", "CertGetCertificateChain", MyCertGetCertificateChain, (LPVOID*)&pOrigCertGetCertificateChain);
#endif
#if defined(_M_IX86)
    Patches::InstallCsgHeapGuard(); // b2: swallow the corrupt CSG temp-buffer free (0.574) so load survives
#endif
    Out("Main", "Hooks created.");

#ifdef NOOBHOOK_HYPERION
    // Hyperion path: skip in-process enable (ACG blocks it).
    // Store hook info so the injector can write JMPs from outside.
    Out("Main", "Storing hook info for external enable...");
    g_HookInfo.count = 0;
    #define STORE_HOOK(created, mod, fn, detourPtr, originalStorage) \
        if ((created) && g_HookInfo.count < 16) { \
            strncpy_s(g_HookInfo.table[g_HookInfo.count].targetModule, mod, sizeof(g_HookInfo.table[0].targetModule)-1); \
            strncpy_s(g_HookInfo.table[g_HookInfo.count].targetFunc, fn, sizeof(g_HookInfo.table[0].targetFunc)-1); \
            g_HookInfo.table[g_HookInfo.count].detourFunc = (void*)(detourPtr); \
            g_HookInfo.table[g_HookInfo.count].originalFuncStorage = reinterpret_cast<void**>(originalStorage); \
            g_HookInfo.count++; \
        }

    STORE_HOOK(madeConnect, "ws2_32.dll", "connect", MyConnect, &pOrigConnect);
    STORE_HOOK(madeWSAConnect, "ws2_32.dll", "WSAConnect", MyWSAConnect, &pOrigWSAConnect);
    STORE_HOOK(madeWSPConnect, "mswsock.dll", "WSPConnect", MyWSPConnect, &pOrigWSPConnect);
    STORE_HOOK(madeWinHttpConnect, "winhttp.dll", "WinHttpConnect", MyWinHttpConnect, &pOrigWinHttpConnect);
    STORE_HOOK(madeWinHttpSendRequest, "winhttp.dll", "WinHttpSendRequest", MyWinHttpSendRequest, &pOrigWinHttpSendRequest);
    STORE_HOOK(madeCreateFileW, "kernel32.dll", "CreateFileW", MyCreateFileW, &pOrigCreateFileW);
    STORE_HOOK(madeCreateFileA, "kernel32.dll", "CreateFileA", MyCreateFileA, &pOrigCreateFileA);
    STORE_HOOK(madeNtCreateFile, "ntdll.dll", "NtCreateFile", MyNtCreateFile, &pOrigNtCreateFile);
    STORE_HOOK(madeNtOpenFile, "ntdll.dll", "NtOpenFile", MyNtOpenFile, &pOrigNtOpenFile);
    STORE_HOOK(madeExitProcess, "kernel32.dll", "ExitProcess", MyExitProcess, &pOrigExitProcess);
    STORE_HOOK(madeRtlExitUserProcess, "ntdll.dll", "RtlExitUserProcess", MyRtlExitUserProcess, &pOrigRtlExitUserProcess);
    STORE_HOOK(madeCertVerify, "crypt32.dll", "CertVerifyCertificateChainPolicy", MyCertVerifyCertificateChainPolicy, &pOrigCertVerifyChainPolicy);
    STORE_HOOK(madeCertGetChain, "crypt32.dll", "CertGetCertificateChain", MyCertGetCertificateChain, &pOrigCertGetCertificateChain);
    STORE_HOOK(madeGetProcAddress, "kernel32.dll", "GetProcAddress", MyGetProcAddress, &pOrigGetProcAddress);

    Out("Main", "Stored %d hooks for injector", g_HookInfo.count);
#else
    // Legacy path: enable hooks normally in-process
    Out("Main", "Enabling hooks in-process...");
    MH_EnableHook(MH_ALL_HOOKS);
    Out("Main", "All hooks enabled");
#endif

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
            std::string path = std::string(dllDir) + "\\noobhook_x86-64_localrcc.dll";
            Out("Main", "Studio binary detected -- loading %s", path.c_str());
            HMODULE localRcc = LoadLibraryA(path.c_str());
            if (localRcc == nullptr) {
                Out("Main", "Failed to load noobhook_x86-64_localrcc.dll (err=%lu)", GetLastError());
            } else {
                using LocalRccInitialize = DWORD (WINAPI*)(LPVOID);
                auto initialize = reinterpret_cast<LocalRccInitialize>(
                    GetProcAddress(localRcc, "NoobLocalRccInitialize"));
                if (initialize == nullptr) {
                    Out("Main", "noobhook_x86-64_localrcc.dll has no initializer (err=%lu)",
                        GetLastError());
                } else {
                    const DWORD result = initialize(localRcc);
                    if (result == ERROR_SUCCESS)
                        Out("Main", "noobhook_x86-64_localrcc.dll initialized");
                    else
                        Out("Main", "noobhook_x86-64_localrcc.dll initialization failed: %lu",
                            result);
                }
            }
        }
    }
#endif

#ifndef NOOBHOOK_HYPERION
    // pretty hard to pull off with a hyperion client.
    //ScriptExecutor::Install();
    Patches::InstallCrashDiagnostics();
#endif

    Out("Main", "Done");
    //fclose(file);

    return 0;
}

BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD reason, LPVOID lpReserved) {
    HANDLE hThread = NULL;
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        {
            wchar_t configuredLog[MAX_PATH] = {};
            DWORD configuredLogLength = GetEnvironmentVariableW(
                L"NOOBHOOK_LOG_PATH", configuredLog, _countof(configuredLog));
            if (configuredLogLength > 0 && configuredLogLength < _countof(configuredLog))
                gFile = _wfreopen(configuredLog, L"w", stdout);
            else
                gFile = freopen("noobhook.log", "w", stdout);
        }
        if (gFile == nullptr) {
            MessageBoxA(NULL, "Failed to open log file for writing.", "noobHook", MB_ICONWARNING | MB_OK);
        }

#if defined(_M_IX86)
        Patches::InstallClusterNullGuard(); // survive the player's corrupt-union cluster crashes
        Patches::InstallUnionRenderUnlock(); // experiment: un-gate the new-vertex-format render path so 2026 unions display
#endif
        Out("DllMain", "Applying patches...");
        Patches::RemoveTrustCheck();
        Patches::RemoveSignatureCheck();
        Patches::RemoveTLSVerification();
        Patches::FixSettingsKeyMustBeDefined();
        Patches::FixInsertObjects();
#if defined(_M_X64)
        Patches::InstallTrampolineIntegrityBypass();
#endif

        DisableThreadLibraryCalls(hModule);

#ifdef NOOBHOOK_HYPERION
        // Hyperion: run synchronously to avoid thread-creation detection
        Thread(hModule);
#else
        // Legacy: run in a real thread (needed for LoadLibrary from DllMain to not deadlock)
        {
            HANDLE hThread = CreateThread(0, 0, Thread, hModule, CREATE_SUSPENDED, 0);
            SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);
            ResumeThread(hThread);
            CloseHandle(hThread);
        }
#endif
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
