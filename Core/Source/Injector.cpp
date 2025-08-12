// === noobWarrior ===
// File: Injector.cpp
// Started by: Hattozo
// Started on: 3/16/2025
// Description: A really basic DLL injector. Anyone could make this, it's not really impressive.
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Config.h>
#include <NoobWarrior/Log.h>

#if defined(_WIN32)
#include <windows.h>
#endif

#include <filesystem>

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

static std::string LastErrorStr(DWORD err = GetLastError()) {
    char buf[512] = {0};
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   buf, (DWORD)sizeof(buf), nullptr);
    return std::string(buf);
}

ClientLaunchResponse Core::LaunchInjectProcess(const std::filesystem::path &filePath) {
    // make this use widechar because the dumbasses at microsoft didn't know that utf 8 would exist
    std::wstring wargs = filePath.wstring() + L" -console -verbose -placeid:1818 -port 53641";
    std::vector<wchar_t> wargs_vec(wargs.begin(), wargs.end());
    wargs_vec.push_back(L'\0');

    std::string args( wargs.begin(), wargs.end() );

    const std::filesystem::path &dllPath = std::filesystem::path(GetInstallationDir() / "noobhook_x86.dll");
    const std::string &dllPathStr = dllPath.string();
    Out("Inject", "Launching process \"{}\" with args \"{}\", will hook DLL file located at \"{}\"", filePath.string(), args, dllPathStr);
#if defined(_WIN32)
    PROCESS_INFORMATION pi {};
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    if (!CreateProcessW(nullptr, wargs_vec.data(), nullptr, nullptr, FALSE, CREATE_SUSPENDED, nullptr, nullptr, &si, &pi)) {
        DWORD err = GetLastError();
        Out("Inject", "CreateProcessW failed: {} ({})", err, LastErrorStr(err));
        return ClientLaunchResponse::FailedToCreateProcess;
    }
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
