// === noobWarrior ===
// File: Injector.cpp
// Started by: Hattozo
// Started on: 3/16/2025
// Description: Source file that implements the code for injecting DLLs into a program.
// Note: This relies heavily on the Windows API. On other operating systems, Wine will be called to execute a Win32 program in order to translate these calls.
#include "NoobWarrior/RobloxClient.h"
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Config.h>
#include <NoobWarrior/Log.h>

#if defined(_WIN32)
#include <windows.h>
#include <tlhelp32.h>
#endif

#include <filesystem>

using namespace NoobWarrior;

static int GetDllBitness(const wchar_t* dllPath) {
#if defined(_WIN32)
    HANDLE hFile = CreateFileW(dllPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return -1; // Error

    HANDLE hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (hMap == NULL) {
        CloseHandle(hFile);
        return -1;
    }

    LPVOID lpBase = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    if (lpBase == NULL) {
        CloseHandle(hMap);
        CloseHandle(hFile);
        return -1;
    }

    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)lpBase;
    if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        UnmapViewOfFile(lpBase);
        CloseHandle(hMap);
        CloseHandle(hFile);
        return -1;
    }

    PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((BYTE*)lpBase + pDosHeader->e_lfanew);
    if (pNtHeaders->Signature != IMAGE_NT_SIGNATURE) {
        UnmapViewOfFile(lpBase);
        CloseHandle(hMap);
        CloseHandle(hFile);
        return -1;
    }

    int bitness = 0;

    switch (pNtHeaders->FileHeader.Machine) {
    case IMAGE_FILE_MACHINE_I386:
        bitness = 32;
        break;
    case IMAGE_FILE_MACHINE_AMD64:
        bitness = 64;
        break;
    default:
        bitness = 0; // Unknown or other architecture
        break;
    }

    UnmapViewOfFile(lpBase);
    CloseHandle(hMap);
    CloseHandle(hFile);

    return bitness;
#else
    return -1;
#endif
}

static std::string LastErrorStr(DWORD err = GetLastError()) {
    char buf[512] = {0};
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   buf, (DWORD)sizeof(buf), nullptr);
    return std::string(buf);
}

ClientLaunchResponse Core::Inject(unsigned long pid, const wchar_t *dllPath) {
    ClientLaunchResponse res = ClientLaunchResponse::InjectFailed;

    if (!std::filesystem::exists(dllPath)) {
        Out("Inject", "DLL does not exist: {}", std::filesystem::path(dllPath).string());
        return ClientLaunchResponse::InjectDllMissing;
    }

    Out("Inject", "Starting injection - PID: {}, DLL: {}", pid, std::filesystem::path(dllPath).string());
#if defined(_WIN32)
    HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (handle == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        Out("Inject", "OpenProcess failed: {} ({})", err, LastErrorStr(err));
        return ClientLaunchResponse::InjectCannotAccessProcess;
    }

    BOOL isWow64;
    IsWow64Process(handle, &isWow64);
    int dllBitness = GetDllBitness(dllPath);

    Out("Inject", "Target WOW64: {}, DLL bitness: {}", isWow64 ? "yes" : "no", dllBitness);

    if (isWow64 && dllBitness != 32) {
        Out("Inject", "Architecture mismatch");
        CloseHandle(handle);
        return ClientLaunchResponse::InjectWrongArchitecture;
    }

    SIZE_T size_bytes = (wcslen(dllPath) + 1) * sizeof(wchar_t);
    void *mem = VirtualAllocEx(handle, NULL, size_bytes, MEM_COMMIT, PAGE_READWRITE);
    if (!mem) {
        DWORD err = GetLastError();
        Out("Inject", "VirtualAllocEx failed: {} ({})", err, LastErrorStr(err));
        CloseHandle(handle);
        return ClientLaunchResponse::InjectCannotWriteToProcessMemory;
    }

    if (!WriteProcessMemory(handle, mem, dllPath, size_bytes, NULL)) {
        DWORD err = GetLastError();
        Out("Inject", "WriteProcessMemory failed: {} ({})", err, LastErrorStr(err));
        res = ClientLaunchResponse::InjectCannotWriteToProcessMemory;
        goto cleanup;
    }

    HMODULE hKernel32;
    hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!hKernel32) {
        Out("Inject", "Failed to get kernel32 handle");
        res = ClientLaunchResponse::InjectFailed;
        goto cleanup;
    }

    FARPROC pLoadLibraryW;
    pLoadLibraryW = GetProcAddress(hKernel32, "LoadLibraryW");
    if (!pLoadLibraryW) {
        Out("Inject", "Failed to get LoadLibraryW address");
        res = ClientLaunchResponse::InjectFailed;
        goto cleanup;
    }

    HANDLE thread;
    thread = CreateRemoteThread(handle, nullptr, 0, (LPTHREAD_START_ROUTINE)pLoadLibraryW, mem, 0, nullptr);
    if (thread == nullptr) {
        DWORD err = GetLastError();
        Out("Inject", "CreateRemoteThread failed: {} ({})", err, LastErrorStr(err));
        res = ClientLaunchResponse::InjectCannotCreateThreadInProcess;
        goto cleanup;
    }
    if (WaitForSingleObject(thread, 5000) != WAIT_OBJECT_0) {
        Out("Inject", "Thread wait failed or timed out");
        res = ClientLaunchResponse::InjectFailed;
        goto cleanup;
    }

    DWORD dwExitCode;
    if (!GetExitCodeThread(thread, &dwExitCode)) {
        DWORD err = GetLastError();
        Out("Inject", "GetExitCodeThread failed: {} ({})", err, LastErrorStr(err));
        res = ClientLaunchResponse::InjectCouldNotGetReturnValueOfLoadLibrary;
        goto cleanup;
    }

    if (dwExitCode == 0) {
        Out("Inject", "LoadLibraryW returned NULL - injection failed");
        res = ClientLaunchResponse::InjectFailedToLoadLibrary;
        goto cleanup;
    }

    res = ClientLaunchResponse::Success;
cleanup:
    if (mem) VirtualFreeEx(handle, mem, 0, MEM_RELEASE);
    if (thread) CloseHandle(thread);
    CloseHandle(handle);
    return res;
#else
    return res;
#endif
}

ClientLaunchResponse Core::LaunchInjectProcess(const std::filesystem::path &filePath) {
    std::wstring wargs = filePath.wstring() + L" -console -verbose -placeid:1818 -port 53641";
    std::vector<wchar_t> wargs_vec(wargs.begin(), wargs.end());
    wargs_vec.push_back(L'\0');

    const std::filesystem::path &dllPath = std::filesystem::path(GetInstallationDir() / "noobhook_x86.dll");
    const std::wstring &dllPathStr = dllPath.wstring();
#if defined(_WIN32)
    PROCESS_INFORMATION pi {};
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    if (!CreateProcessW(nullptr, wargs_vec.data(), nullptr, nullptr, FALSE, CREATE_SUSPENDED, nullptr, nullptr, &si, &pi)) {
        DWORD err = GetLastError();
        Out("Inject", "CreateProcessW failed: {} ({})", err, LastErrorStr(err));
        return ClientLaunchResponse::FailedToCreateProcess;
    }
    ClientLaunchResponse inject = Inject(GetProcessId(pi.hProcess), dllPathStr.c_str());
    if (inject != ClientLaunchResponse::Success) {
        TerminateProcess(pi.hProcess, 0xEEEEEEEE);
        return inject;
    }
    if (ResumeThread(pi.hThread) == -1) {   
        TerminateProcess(pi.hProcess, 0xFFFFFFFF);
        CloseHandle(pi.hThread);
        return ClientLaunchResponse::InjectFailedToResumeProcess;
    }
    // HANDLE waitHandle;
    // RegisterWaitForSingleObject(&waitHandle, pi.hProcess, &ProcessExitCallback, NULL, INFINITE, WT_EXECUTEONLYONCE);
    // CloseHandle(pi.hProcess);
    return ClientLaunchResponse::Success;
#elif defined(__unix__) || defined(__APPLE__)
    // where wine comes in
    return ClientLaunchResponse::Failed;
#endif
}
