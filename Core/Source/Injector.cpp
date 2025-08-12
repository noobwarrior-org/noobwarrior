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

ClientLaunchResponse Core::LaunchInjectProcess(const std::filesystem::path &filePath) {
    std::string args = filePath.string() + " -console -verbose -placeid:1818 -port 53641";
    const std::filesystem::path &dllPath = std::filesystem::path(GetInstallationDir() / "noobhook_x86.dll");
    const std::string &dllPathStr = dllPath.generic_string();
#if defined(_WIN32)
    PROCESS_INFORMATION pi = { 0 };
    STARTUPINFO si = { 0 };
    si.cb = sizeof(STARTUPINFO);
    if (!CreateProcessA((LPCSTR)filePath.c_str(), (char*)args.c_str(), NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi))
        return ClientLaunchResponse::FailedToCreateProcess;
    if (!Inject(GetProcessId(pi.hProcess), const_cast<char*>(dllPathStr.c_str())))
        return ClientLaunchResponse::FailedToInject;
    if (ResumeThread(pi.hThread) == -1)
        return ClientLaunchResponse::Failed;
    // HANDLE waitHandle;
    // RegisterWaitForSingleObject(&waitHandle, pi.hProcess, &ProcessExitCallback, NULL, INFINITE, WT_EXECUTEONLYONCE);
    CloseHandle(pi.hProcess);
    return ClientLaunchResponse::Success;
#elif defined(__unix__) || defined(__APPLE__)
    // where wine comes in
    return ClientLaunchResponse::Failed;
#endif
}
