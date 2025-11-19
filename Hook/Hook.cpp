// === noobWarrior ===
// File: Hook.cpp
// Started by: Hattozo
// Started on: 3/16/2025
// Description: Contains main entrypoint for noobWarrior Roblox hook
// Please do not use any standard libraries in this other than the Windows API. Thank you.
#include "Hook.h"

#include <MinHook.h>

#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <strsafe.h>

enum RobloxVersion {
    VER_UNKNOWN,
    VER_0_449_0_411458,
    VER_0_463_0_417004
};

enum CURLoption {
    CURLOPT_URL = 10000 + 2
};

DWORD StrLength(PCHAR str) {
    DWORD length = 0;
    while (str[length] != '\0') {
        length++;
    }
    return length;
}

DWORD FindPattern(BYTE* mem, DWORD mem_size, LPCSTR pattern, LPCSTR mask) {
    DWORD pattern_size = StrLength(const_cast<PCHAR>(pattern));
    for (DWORD mem_index = 0; mem_index < mem_size; mem_index++) {
        BOOL matches = true;
        for (DWORD pattern_index = 0; pattern_index < pattern_size; pattern_index++) {
            const CHAR byte_pattern = pattern[pattern_index];
            const CHAR byte_mem = mem[mem_index + pattern_index];

            if (byte_pattern != byte_mem) {
                matches = false;
                break;
            }
        }
        if (matches) return mem_index;
    }
    return 0x0;
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

DWORD WINAPI Thread(LPVOID param) {
    // every other thread please fck off and let us do our job. thanks.
    SuspendAllThreadsExceptMines(GetCurrentProcessId(), GetCurrentThreadId());
    HMODULE mod = GetModuleHandle(NULL);
    MODULEINFO modInfo { 0 };
    BOOL modInfoSuccess = GetModuleInformation(GetCurrentProcess(), mod, &modInfo, sizeof(MODULEINFO));

    LPVOID modBase = modInfo.lpBaseOfDll;
    DWORD modSize = modInfo.SizeOfImage;

    BYTE* mem = new BYTE[modSize];
    WINBOOL readSuccess = ReadProcessMemory(GetCurrentProcess(), modBase, mem, modSize, NULL);

    if (!readSuccess) {
        delete[] mem;
        MessageBoxA(NULL, "noobHook failed to read the memory of the process it was trying to inject to. Exiting!", "noobHook", MB_ICONERROR | MB_OK);
        TerminateProcess(GetCurrentProcess(), 0xB16B00B5);
        return 0xB16B00B5;
    }

    RobloxVersion ver = GetRobloxVersion();
    if (ver == VER_UNKNOWN) {
        MessageBoxA(NULL, "noobHook does not support this version of Roblox. Exiting!", "noobHook", MB_ICONERROR | MB_OK);
        TerminateProcess(GetCurrentProcess(), 0xDEADBEEF);
        return 0xDEADBEEF;
    } else if (ver == VER_0_463_0_417004) {
        // Bypasses "Settings key must be defined" error for RCCService. I have no idea why it does this.
        DWORD signature_address = FindPattern(mem, modSize, "\x0F\xB6\xC8\x85\xC9\x74\x2A", "xxxxxxx");
        DWORD full_signature_address = reinterpret_cast<DWORD>(modBase) + signature_address;
        DWORD address = full_signature_address + 6;

        for (int i = 0; i < 8; i++) {
            int poop = *(reinterpret_cast<int*>(full_signature_address + i));

            TCHAR szBuffer1[4];
            HRESULT hr1 = StringCchPrintf(szBuffer1, ARRAYSIZE(szBuffer1), TEXT("%i "), poop);

            HANDLE stdOut1 = GetStdHandle(STD_OUTPUT_HANDLE);
            WriteConsoleA(stdOut1, szBuffer1, ARRAYSIZE(szBuffer1), NULL, NULL);
        }

        DWORD old_protection;
        VirtualProtect(reinterpret_cast<LPVOID>(address), 1, PAGE_EXECUTE_READWRITE, &old_protection);
        memset(reinterpret_cast<LPVOID>(address), 0xEB, 1);
        VirtualProtect(reinterpret_cast<LPVOID>(address), 1, old_protection, NULL);

        TCHAR szBuffer[20];
        HRESULT hr = StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("Base Address 0x%08x"), signature_address);

        HANDLE stdOut = GetStdHandle(STD_OUTPUT_HANDLE);
        WriteConsoleA(stdOut, szBuffer, ARRAYSIZE(szBuffer), NULL, NULL);

        MessageBoxA(NULL, signature_address != NULL ? "Found base address!" : "Could not find base address. :(", "noobHook", MB_ICONASTERISK | MB_OK);

        // MH_CreateHook();
        
        MessageBoxA(NULL, "I am running on version 0.463.0.417004", "noobHook", MB_ICONASTERISK | MB_OK);
    } else if (ver == VER_0_449_0_411458) {
        MessageBoxA(NULL, "I am running on version 0.449.0.411458", "noobHook", MB_ICONASTERISK | MB_OK);
    }
    delete[] mem;
    ResumeAllThreadsExceptMines(GetCurrentProcessId(), GetCurrentThreadId());
    return 0;
}

BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD reason, LPVOID lpReserved) {
    HANDLE hThread = NULL;
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        MH_Initialize();
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