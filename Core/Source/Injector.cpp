// === noobWarrior ===
// File: Injector.cpp
// Started by: Hattozo
// Started on: 3/16/2025
// Description: A really basic DLL injector. Anyone could make this, it's not really impressive.
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Config.h>
#include <filesystem>

#if defined(_WIN32)
#include <windows.h>
#endif

#include <format>

using namespace NoobWarrior;

int Core::Inject(unsigned long pid, char *dllPath) {
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

// #if defined(_WIN32)
// CALLBACK ProcessExitCallback(PVOID lpParam, BOOLEAN timerFired) {

// }
// #endif

int Core::LaunchInjectProcess(const std::filesystem::path &filePath) {
#if defined(_WIN32)
    PROCESS_INFORMATION pi = { 0 };
    STARTUPINFO si = { 0 };
    si.cb = sizeof(STARTUPINFO);
    if (!CreateProcessA((LPCSTR)filePath.c_str(), NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi))
        return -2;
    if (!Inject(GetProcessId(pi.hProcess), (char*)"./noobwarrior-hook.dll"))
        return -1;
    if (ResumeThread(pi.hThread) == -1)
        return 0;
    // HANDLE waitHandle;
    // RegisterWaitForSingleObject(&waitHandle, pi.hProcess, &ProcessExitCallback, NULL, INFINITE, WT_EXECUTEONLYONCE);
    CloseHandle(pi.hProcess);
    return 1;
#elif defined(__unix__) || defined(__APPLE__)
    // where wine comes in
    return 0;
#endif
}
