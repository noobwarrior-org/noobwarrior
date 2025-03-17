// === noobWarrior ===
// File: Injector.cpp
// Started by: Hattozo
// Started on: 3/16/2025
// Description: Injects hook DLL into Roblox process
#include <NoobWarrior/Injector.h>

#if defined(_WIN32)
#include <windows.h>
#endif

using namespace NoobWarrior;

int NoobWarrior::Inject(unsigned long pid, char *dllPath) {
#if defined(_WIN32)
    HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (handle == NULL)
        return 0;
    void *mem = VirtualAllocEx(handle, NULL, strlen(dllPath) + 1, MEM_COMMIT, PAGE_READWRITE);
    WriteProcessMemory(handle, mem, (void*)dllPath, strlen(dllPath) + 1, NULL);
    HANDLE thread = CreateRemoteThread(handle, NULL, 0, (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle("kernel32"), "LoadLibraryA"), mem, 0, NULL);
    WaitForSingleObject(thread, INFINITE);
    VirtualFreeEx(handle, mem, strlen(dllPath) + 1, MEM_RELEASE);
    CloseHandle(handle);
    return 1;
#else
    return 0;
#endif
}