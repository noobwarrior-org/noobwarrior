// === noobWarrior ===
// File: Hook.cpp
// Started by: Hattozo
// Started on: 3/16/2025
// Description: Contains main entrypoint for noobWarrior Roblox hook
#include "Hook.h"

#include <windows.h>
#include <psapi.h>

enum RobloxVersion {
    VER_UNKNOWN,
    VER_0_449_0_411458
};

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
    MessageBoxA(NULL, ver, "noobWarrior", MB_ICONASTERISK | MB_OK);
    if (strncmp(ver, "0, 449, 0, 411458", strlen(ver)) == 0) {
        return VER_0_449_0_411458;
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

DWORD WINAPI Thread(LPVOID param) {
    /*
    switch (GetRobloxVersion()) {
    case VER_UNKNOWN:
        MessageBoxA(NULL, "I have no idea what version I am running on!", "noobWarrior", MB_ICONASTERISK | MB_OK);
        break;
    case VER_0_449_0_411458:
        MessageBoxA(NULL, "I am running on version 0.449.0.411458", "noobWarrior", MB_ICONASTERISK | MB_OK);
        break;
    }
    */
    MessageBoxA(NULL, "hi lol", "noobWarrior", MB_ICONASTERISK | MB_OK);
    // TerminateProcess(GetCurrentProcess(), 1);
    return 0;
}

BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD reason, LPVOID lpReserved) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        CreateThread(0, 0, Thread, hModule, 0, 0);
        break;
    case DLL_PROCESS_DETACH:
        if (lpReserved != nullptr)
            break;
        break;
    default:
        break;
    }
    return TRUE;
}