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
// File: ManualMap.cpp
// Started by: Hattozo
// Started on: 8/12/2025
// Description: Manual mapper - TP_TASK_CBS + NtSetIoCompletion
// Slopped by DeepSeek V4 Pro and GPT 5.6 Sol.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winternl.h>
#include <psapi.h>
#include "ManualMap.h"
#include "Bypass.h"
#include "Syscalls.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

// A five-byte x64 JMP can only reach +/-2 GB.  The hook DLL is manually mapped, so letting
// NtAllocateVirtualMemory choose an arbitrary address can put it down near the normal heap while
// ws2_32/winhttp/kernel32 live at the top of the user address space.  The injector later enables
// the MinHook detours with relative JMPs, so reserve the image near the target's system DLLs.
static void* AllocateImageNearSystemDlls(HANDLE hProcess, SIZE_T imageSize)
{
    HMODULE modules[1024] = {};
    DWORD moduleBytes = 0;
    uintptr_t anchor = 0;

    if (EnumProcessModules(hProcess, modules, sizeof(modules), &moduleBytes)) {
        const DWORD moduleCount = min(moduleBytes / sizeof(HMODULE), (DWORD)_countof(modules));
        for (DWORD i = 0; i < moduleCount; ++i) {
            wchar_t name[MAX_PATH] = {};
            if (GetModuleBaseNameW(hProcess, modules[i], name, _countof(name)) &&
                _wcsicmp(name, L"kernel32.dll") == 0) {
                anchor = reinterpret_cast<uintptr_t>(modules[i]);
                break;
            }
        }
    }

    if (!anchor)
        return nullptr;

    SYSTEM_INFO systemInfo = {};
    GetSystemInfo(&systemInfo);
    const uintptr_t granularity = systemInfo.dwAllocationGranularity;
    const uintptr_t reachWithMargin = 0x70000000ULL; // 1.75 GB leaves room for DLL spread/image RVAs.
    const uintptr_t lower = anchor > reachWithMargin ? anchor - reachWithMargin : granularity;
    const uintptr_t maxAddress = reinterpret_cast<uintptr_t>(systemInfo.lpMaximumApplicationAddress);
    const uintptr_t upper = anchor <= maxAddress - reachWithMargin
        ? anchor + reachWithMargin : maxAddress;

    for (uintptr_t cursor = lower; cursor < upper;) {
        MEMORY_BASIC_INFORMATION mbi = {};
        if (!VirtualQueryEx(hProcess, reinterpret_cast<void*>(cursor), &mbi, sizeof(mbi)))
            break;

        const uintptr_t regionBase = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
        const uintptr_t regionEnd = regionBase + mbi.RegionSize;
        if (mbi.State == MEM_FREE) {
            const uintptr_t candidate = (max(cursor, regionBase) + granularity - 1) & ~(granularity - 1);
            if (candidate < upper && regionEnd >= candidate && regionEnd - candidate >= imageSize) {
                void* requested = reinterpret_cast<void*>(candidate);
                SIZE_T size = imageSize;
                NTSTATUS status = SysNtAllocateVirtualMemory(
                    hProcess, &requested, 0, &size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
                if (status >= 0 && requested) {
                    printf("[MM] Reserved image near system DLLs (anchor=0x%llX, base=0x%p)\n",
                        (unsigned long long)anchor, requested);
                    return requested;
                }
            }
        }

        if (regionEnd <= cursor)
            break;
        cursor = regionEnd;
    }

    return nullptr;
}

#pragma pack(push, 1)
struct ShellcodeData {
    void* pLoadLibraryA;
    void* pGetProcAddress;
    void* pRtlAddFunctionTable;
    uint8_t* imageBase;
    void* hMod;
    DWORD  reason;
    void* reserved;
    bool   sehSupport;
};
#pragma pack(pop)

std::vector<uint8_t> ReadDllFile(const wchar_t* path)
{
    HANDLE h = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) return {};
    DWORD sz = GetFileSize(h, NULL);
    std::vector<uint8_t> data(sz);
    DWORD r;
    ReadFile(h, data.data(), sz, &r, NULL);
    CloseHandle(h);
    return data;
}

static DWORD WINAPI ShellcodeEntry(ShellcodeData* pData)
{
    if (!pData || !pData->imageBase) return 0;
    auto* dos = (PIMAGE_DOS_HEADER)pData->imageBase;
    auto* nt  = (PIMAGE_NT_HEADERS64)(pData->imageBase + dos->e_lfanew);
    uint8_t* base = pData->imageBase;
    auto pLoadLib = (decltype(&LoadLibraryA))pData->pLoadLibraryA;
    auto pGetProc = (decltype(&GetProcAddress))pData->pGetProcAddress;

    uintptr_t delta = (uintptr_t)base - nt->OptionalHeader.ImageBase;
    if (delta) {
        auto& rdir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
        if (rdir.VirtualAddress && rdir.Size) {
            auto* reloc = (IMAGE_BASE_RELOCATION*)(base + rdir.VirtualAddress);
            while (reloc->SizeOfBlock) {
                DWORD cnt = (reloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
                auto* ents = (WORD*)(reloc + 1);
                for (DWORD j = 0; j < cnt; j++)
                    if (ents[j] && (ents[j] >> 12) == IMAGE_REL_BASED_DIR64)
                        *(uintptr_t*)(base + reloc->VirtualAddress + (ents[j] & 0xFFF)) += delta;
                reloc = (IMAGE_BASE_RELOCATION*)((uint8_t*)reloc + reloc->SizeOfBlock);
            }
        }
    }

    auto& idir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (idir.VirtualAddress && idir.Size) {
        auto* imp = (IMAGE_IMPORT_DESCRIPTOR*)(base + idir.VirtualAddress);
        for (; imp->Name; imp++) {
            HMODULE mod = pLoadLib((const char*)(base + imp->Name));
            if (!mod) { pData->hMod = (void*)0x404040; return 0; }
            auto* th = (IMAGE_THUNK_DATA64*)(base + (imp->OriginalFirstThunk ? imp->OriginalFirstThunk : imp->FirstThunk));
            auto* fd = (IMAGE_THUNK_DATA64*)(base + imp->FirstThunk);
            for (; th->u1.AddressOfData; th++, fd++) {
                uintptr_t addr = 0;
                if (th->u1.Ordinal & IMAGE_ORDINAL_FLAG64)
                    addr = (uintptr_t)pGetProc(mod, MAKEINTRESOURCEA(IMAGE_ORDINAL64(th->u1.Ordinal)));
                else { auto* bn = (IMAGE_IMPORT_BY_NAME*)(base + th->u1.AddressOfData); addr = (uintptr_t)pGetProc(mod, bn->Name); }
                if (addr) *(uintptr_t*)fd = addr;
            }
        }
    }

    auto& tdir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];
    if (tdir.VirtualAddress && tdir.Size) {
        auto* tls = (IMAGE_TLS_DIRECTORY64*)(base + tdir.VirtualAddress);
        if (tls->AddressOfCallBacks) { auto* cb = (PIMAGE_TLS_CALLBACK*)(tls->AddressOfCallBacks); for (; *cb; cb++) (*cb)(base, DLL_PROCESS_ATTACH, nullptr); }
    }

    if (pData->sehSupport && pData->pRtlAddFunctionTable) {
        auto& edir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
        if (edir.VirtualAddress && edir.Size) {
            auto pAdd = (decltype(&RtlAddFunctionTable))pData->pRtlAddFunctionTable;
            pAdd((PRUNTIME_FUNCTION)(base + edir.VirtualAddress), edir.Size / sizeof(RUNTIME_FUNCTION), (DWORD64)base);
        }
    }

    using DllMain_t = BOOL(WINAPI*)(HINSTANCE, DWORD, LPVOID);
    auto dm = (DllMain_t)(base + nt->OptionalHeader.AddressOfEntryPoint);
    if (dm) dm((HINSTANCE)base, pData->reason, pData->reserved);
    pData->hMod = base;
    return 0;
}

// --- IoCompletion handle finder (same as before) ---

struct HandleEntry { ULONG_PTR pid; ULONG_PTR handle; };

static std::vector<HANDLE> FindIoCompletionHandles(HANDLE hProcess, DWORD pid)
{
    std::vector<HANDLE> result;
    // Use ProcessHandleInformation (class 51) instead
    ULONG classes[] = { 51, 0x40, 0x33 };

    for (ULONG cls : classes) {
        ULONG bufSize = 0;
        NTSTATUS st = SysNtQuerySystemInformation(cls, nullptr, 0, &bufSize);
        if (!bufSize) bufSize = 4 * 1024 * 1024;

        for (int retry = 0; retry < 3; retry++) {
            std::vector<uint8_t> buf(bufSize + 0x100000);
            st = SysNtQuerySystemInformation(cls, buf.data(), bufSize + 0x100000, &bufSize);
            if (st == 0xC0000004) continue;
            if (st < 0) break;

            struct SysHandleInfo {
                ULONG_PTR NumberOfHandles; ULONG_PTR Reserved;
                struct { PVOID Object; ULONG_PTR UniqueProcessId; ULONG_PTR HandleValue; ULONG GrantedAccess; USHORT CBT; USHORT OTI; ULONG HandleAttributes; ULONG Reserved; } Handles[1];
            };
            auto* info = (SysHandleInfo*)buf.data();
            printf("[MM] Scanning %llu handles (class 0x%lX)\n", (unsigned long long)info->NumberOfHandles, cls);

            for (ULONG_PTR i = 0; i < info->NumberOfHandles && result.size() < 10; i++) {
                auto& e = info->Handles[i];
                if (e.UniqueProcessId != pid || !e.OTI) continue;
                HANDLE dup = nullptr;
                SysNtDuplicateObject(hProcess, (HANDLE)e.HandleValue, GetCurrentProcess(), &dup, 0, 0, DUPLICATE_SAME_ACCESS);
                if (!dup) continue;
                uint8_t tb[1024] = {}; ULONG rl = 0;
                st = SysNtQueryObject(dup, 2, tb, sizeof(tb), &rl);
                if (rl > 0) {
                    auto* tn = (UNICODE_STRING*)tb;
                    if (tn->Buffer && tn->Length >= 24) {
                        wchar_t nm[64] = {};
                        wcsncpy_s(nm, tn->Buffer, min((size_t)(tn->Length / 2), (size_t)63));
                        if (wcsstr(nm, L"IoCompletion") && !wcsstr(nm, L"Wait")) {
                            printf("[MM] IoCompletion: %ls\n", nm);
                            result.push_back(dup); dup = nullptr;
                        }
                    }
                }
                if (dup) CloseHandle(dup);
            }
            break;
        }
        if (!result.empty()) break;
    }
    return result;
}

// --- ManualMapDll ---

uintptr_t ManualMapDll(HANDLE hProcess, const std::vector<uint8_t>& dllData)
{
    if (dllData.size() < sizeof(IMAGE_DOS_HEADER)) return 0;
    auto* dd = (const IMAGE_DOS_HEADER*)dllData.data();
    if (dd->e_magic != IMAGE_DOS_SIGNATURE) return 0;
    auto* nd = (const IMAGE_NT_HEADERS64*)(dllData.data() + dd->e_lfanew);
    if (nd->Signature != IMAGE_NT_SIGNATURE) return 0;
    SIZE_T isz = nd->OptionalHeader.SizeOfImage;

    void* remoteBase = nullptr;
    void* pDataMem   = nullptr;
    uintptr_t result = 0;
    SIZE_T wr = 0, rs = 0;
    NTSTATUS st = 0;
    ShellcodeData sd = {};
    PIMAGE_SECTION_HEADER sh = nullptr;
    DWORD pid = 0;
    void* dataBuf = nullptr;
    uintptr_t cbsAddr = 0, dirAddr = 0, shellAddr = 0;
    uintptr_t stompAddr = 0;
    uint8_t stompOriginal[64] = {};
    size_t stompSize = 0;
    bool stompSaved = false;
    std::vector<HANDLE> iocpHandles;
    NTSTATUS (NTAPI* pFn)(HANDLE, ULONG_PTR, void*, NTSTATUS, ULONG_PTR) = nullptr;

    remoteBase = AllocateImageNearSystemDlls(hProcess, isz);
    if (!remoteBase) {
        printf("[MM] Could not reserve the image within rel32 range of the system DLLs\n");
        return 0;
    }
    printf("[MM] Base: 0x%p  Size: 0x%zX\n", remoteBase, isz);

    st = SysNtWriteVirtualMemory(hProcess, remoteBase, dllData.data(), nd->OptionalHeader.SizeOfHeaders, &wr);
    if (st < 0 || wr != nd->OptionalHeader.SizeOfHeaders) goto fail;

    sh = IMAGE_FIRST_SECTION(nd);
    for (WORD i = 0; i < nd->FileHeader.NumberOfSections; i++, sh++) {
        if (!sh->SizeOfRawData) continue;
        st = SysNtWriteVirtualMemory(hProcess, (uint8_t*)remoteBase + sh->VirtualAddress,
                                      dllData.data() + sh->PointerToRawData, sh->SizeOfRawData, &wr);
        if (st < 0 || wr != sh->SizeOfRawData) goto fail;
    }

    // Zero .bss sections (SizeOfRawData == 0 but VirtualSize > 0)
    {
        sh = IMAGE_FIRST_SECTION(nd);
        for (WORD i = 0; i < nd->FileHeader.NumberOfSections; i++, sh++) {
            if (sh->SizeOfRawData != 0 || sh->Misc.VirtualSize == 0) continue;
            std::vector<uint8_t> zeros(sh->Misc.VirtualSize, 0);
            st = SysNtWriteVirtualMemory(hProcess, (uint8_t*)remoteBase + sh->VirtualAddress,
                                          zeros.data(), zeros.size(), &wr);
            if (st < 0) { printf("[MM] Failed to zero .bss section\n"); goto fail; }
        }
    }

    rs = sizeof(ShellcodeData);
    st = SysNtAllocateVirtualMemory(hProcess, &pDataMem, 0, &rs, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (st < 0 || !pDataMem) goto fail;

    sd.pLoadLibraryA   = GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryA");
    sd.pGetProcAddress = GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetProcAddress");
    sd.pRtlAddFunctionTable = GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlAddFunctionTable");
    sd.imageBase = (uint8_t*)remoteBase;
    sd.reason    = DLL_PROCESS_ATTACH;
    sd.sehSupport = true;
    st = SysNtWriteVirtualMemory(hProcess, pDataMem, &sd, sizeof(sd), &wr);
    if (st < 0) goto fail;

    pid = GetProcessId(hProcess);

    // Allocate work buffer: [flag(4)] [pad(12)] [TP_TASK_CBS(16)] [pad(16)] [TP_DIRECT(80)]
    { SIZE_T sz = 0x1000;
      st = SysNtAllocateVirtualMemory(hProcess, &dataBuf, 0, &sz, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
      if (st < 0 || !dataBuf) goto fail;
      std::vector<uint8_t> zz(0x1000, 0);
      SysNtWriteVirtualMemory(hProcess, dataBuf, zz.data(), 0x1000, &wr); }

    cbsAddr = (uintptr_t)dataBuf + 16;
    dirAddr = (uintptr_t)dataBuf + 48;

    // Write shellcode to a non-critical DLL's .text section
    // This gives us a whitelisted execution address
    {
        // Find a non-critical DLL with a .text section
        HMODULE mods[1024]; DWORD n;
        HANDLE hQ = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        const wchar_t* candidates[] = { L"dbghelp.dll", L"msvcrt.dll", L"comctl32.dll", L"shell32.dll", L"user32.dll", L"win32u.dll" };
        
        if (hQ && EnumProcessModules(hQ, mods, sizeof(mods), &n)) {
            for (const wchar_t* cn : candidates) {
                for (DWORD j = 0; j < n/sizeof(HMODULE); j++) {
                    wchar_t nm[64]; GetModuleBaseNameW(hQ, mods[j], nm, 64);
                    if (_wcsicmp(nm, cn) == 0) {
                        // Find .text section
                        IMAGE_DOS_HEADER dos = {};
                        ReadProcessMemory(hProcess, mods[j], &dos, sizeof(dos), nullptr);
                        IMAGE_NT_HEADERS64 nth = {};
                        ReadProcessMemory(hProcess, (LPCVOID)((uintptr_t)mods[j] + dos.e_lfanew), &nth, sizeof(nth), nullptr);
                        auto* sec = IMAGE_FIRST_SECTION(&nth);
                        uintptr_t modAddr = (uintptr_t)mods[j];
                        
                        // Read sections from remote process
                        DWORD secOff = dos.e_lfanew + sizeof(IMAGE_NT_HEADERS64);
                        for (WORD s = 0; s < nth.FileHeader.NumberOfSections; s++) {
                            IMAGE_SECTION_HEADER sh = {};
                            ReadProcessMemory(hProcess, (LPCVOID)(modAddr + secOff + s * sizeof(sh)), &sh, sizeof(sh), nullptr);
                            char snm[9] = {}; memcpy(snm, sh.Name, 8);
                            if (strcmp(snm, ".text") == 0 && sh.Misc.VirtualSize > 256) {
                                // Use last quarter of .text to avoid critical code
                                DWORD offset = sh.VirtualAddress + sh.Misc.VirtualSize * 3 / 4;
                                offset += (GetTickCount() % (sh.Misc.VirtualSize / 4)) & ~0xF; // random, 16-byte align
                                stompAddr = modAddr + offset;
                                printf("[MM] Stomping %ls .text at 0x%llX\n", cn, (unsigned long long)stompAddr);
                                break;
                            }
                        }
                    }
                    if (stompAddr) break;
                }
                if (stompAddr) break;
            }
        }
        if (hQ) CloseHandle(hQ);
        if (!stompAddr) { printf("[MM] No stomp target DLL found\n"); goto fail; }

        // Copy ShellcodeEntry function to remote process
        void* remoteShellcodeEntry = nullptr;
        {
            SIZE_T scSize = 0x2000;
            st = SysNtAllocateVirtualMemory(hProcess, &remoteShellcodeEntry, 0, &scSize,
                MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
            if (st < 0 || !remoteShellcodeEntry) {
                printf("[MM] Failed to alloc shellcode mem: 0x%lX\n", (unsigned long)st);
                goto fail;
            }
            SysNtWriteVirtualMemory(hProcess, remoteShellcodeEntry,
                (void*)&ShellcodeEntry, scSize, &wr);
            printf("[MM] Wrote ShellcodeEntry at 0x%llX (%zu bytes)\n",
                (unsigned long long)remoteShellcodeEntry, scSize);
        }

        // Write stomp shellcode: calls ShellcodeEntry(pDataMem), then sets hMod, then ret
        {
            uint8_t sc[64] = {};
            size_t off = 0;
            uintptr_t hModAddr = (uintptr_t)pDataMem + 0x20;

            sc[off++] = 0x48; sc[off++] = 0x83; sc[off++] = 0xEC; sc[off++] = 0x28; // sub rsp, 0x28
            sc[off++] = 0x48; sc[off++] = 0xB9; memcpy(sc + off, &pDataMem, 8); off += 8; // mov rcx, pDataMem
            sc[off++] = 0x48; sc[off++] = 0xB8; memcpy(sc + off, &remoteShellcodeEntry, 8); off += 8; // mov rax, ShellcodeEntry
            sc[off++] = 0xFF; sc[off++] = 0xD0; // call rax
            sc[off++] = 0x48; sc[off++] = 0xB8; memcpy(sc + off, &hModAddr, 8); off += 8; // mov rax, hModAddr
            sc[off++] = 0x48; sc[off++] = 0xB9; memcpy(sc + off, &remoteBase, 8); off += 8; // mov rcx, remoteBase
            sc[off++] = 0x48; sc[off++] = 0x89; sc[off++] = 0x08; // mov [rax], rcx  (store remoteBase as hMod)
            sc[off++] = 0x48; sc[off++] = 0x83; sc[off++] = 0xC4; sc[off++] = 0x28; // add rsp, 0x28
            sc[off++] = 0xC3; // ret

            SIZE_T saved = 0;
            if (!ReadProcessMemory(hProcess, reinterpret_cast<const void*>(stompAddr),
                                   stompOriginal, off, &saved) || saved != off) {
                printf("[MM] Failed to save stomp target bytes\n");
                goto fail;
            }
            stompSize = off;
            stompSaved = true;

            DWORD oldProt = 0;
            SIZE_T written = 0;
            if (!VirtualProtectEx(hProcess, reinterpret_cast<void*>(stompAddr), off,
                                  PAGE_EXECUTE_READWRITE, &oldProt) ||
                !WriteProcessMemory(hProcess, reinterpret_cast<void*>(stompAddr),
                                    sc, off, &written) || written != off) {
                printf("[MM] Failed to write stomp callback\n");
                goto fail;
            }
            FlushInstructionCache(hProcess, reinterpret_cast<const void*>(stompAddr), off);
            DWORD ignored = 0;
            VirtualProtectEx(hProcess, reinterpret_cast<void*>(stompAddr), off,
                             oldProt, &ignored);
            printf("[MM] Stomped %zu bytes\n", off);
        }

        // Set TP_TASK_CBS.Exec = stompAddr
        {
            struct { void* Exec; void* Unpost; } cbs = { (void*)stompAddr, nullptr };
            SysNtWriteVirtualMemory(hProcess, (void*)cbsAddr, &cbs, sizeof(cbs), &wr);
        }
        // Set TP_DIRECT Callback = stompAddr
        {
            struct TpDirFix {
                struct { void* Cbs; UINT32 Numa; UINT8 Cpu; char p1[3]; LIST_ENTRY List; } Task;
                UINT64 Lock; LIST_ENTRY IoList; void* Callback; UINT32 N2; UINT8 C2; char p2[3];
            } dir = {};
            dir.Task.Cbs = (void*)cbsAddr;
            dir.Callback = (void*)stompAddr;
            SysNtWriteVirtualMemory(hProcess, (void*)dirAddr, &dir, sizeof(dir), &wr);
        }

        printf("[MM] TP_TASK_CBS Exec=0x%llX (stomped DLL .text)\n", (unsigned long long)stompAddr);
        printf("[MM] TP_DIRECT at 0x%llX (Cbs=0x%llX)\n", (unsigned long long)dirAddr, (unsigned long long)cbsAddr);
    }

    // Find IoCompletion handles and trigger the work item
    {
        iocpHandles = FindIoCompletionHandles(hProcess, pid);
        printf("[MM] %zu IoCompletion handles\n", iocpHandles.size());
        if (iocpHandles.empty()) goto fail;

        pFn = (NTSTATUS(NTAPI*)(HANDLE, ULONG_PTR, void*, NTSTATUS, ULONG_PTR))
                   GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtSetIoCompletion");
        if (!pFn) goto fail;

        for (HANDLE hx : iocpHandles) {
            NTSTATUS s = pFn(hx, (ULONG_PTR)dirAddr, pDataMem, 0, 0);
            CloseHandle(hx);
            printf("[MM] NtSetIoCompletion(dirAddr=0x%llX): 0x%lX\n", (unsigned long long)dirAddr, (unsigned long)s);
            if (s >= 0) {
                for (int w = 0; w < 400; w++) {
                    ShellcodeData ck = {};
                    SysNtReadVirtualMemory(hProcess, pDataMem, &ck, sizeof(ck), nullptr);
                    if (ck.hMod) { result = (uintptr_t)remoteBase; printf("[MM] DONE! Shellcode ran!\n"); break; }
                    Sleep(50);
                }
                if (result) break;
            }
        }
        if (!result) printf("[MM] Timed out\n");
    }

    if (result && stompSaved) {
        // hMod is stored immediately before the callback restores RSP and
        // returns. Give that worker a scheduling quantum, then put the exact
        // original instructions back. The staged mapper previously left its
        // 54-byte callback in dbghelp.dll permanently.
        Sleep(10);
        DWORD oldProt = 0;
        SIZE_T restored = 0;
        if (VirtualProtectEx(hProcess, reinterpret_cast<void*>(stompAddr), stompSize,
                             PAGE_EXECUTE_READWRITE, &oldProt) &&
            WriteProcessMemory(hProcess, reinterpret_cast<void*>(stompAddr),
                               stompOriginal, stompSize, &restored) &&
            restored == stompSize) {
            FlushInstructionCache(hProcess, reinterpret_cast<const void*>(stompAddr),
                                  stompSize);
            DWORD ignored = 0;
            VirtualProtectEx(hProcess, reinterpret_cast<void*>(stompAddr), stompSize,
                             oldProt, &ignored);
            printf("[MM] Restored %zu original stomp bytes at 0x%llX\n",
                   stompSize, (unsigned long long)stompAddr);
        } else {
            printf("[MM] WARNING: failed to restore stomp target bytes (error %lu)\n",
                   GetLastError());
        }
    }

    if (!result) printf("[MM] Failed\n");
    return result;

fail:
    printf("[MM] Failed\n");
    if (remoteBase) { rs = 0; SysNtFreeVirtualMemory(hProcess, &remoteBase, &rs, MEM_RELEASE); }
    if (pDataMem)   { rs = 0; SysNtFreeVirtualMemory(hProcess, &pDataMem,   &rs, MEM_RELEASE); }
    if (dataBuf)    { rs = 0; SysNtFreeVirtualMemory(hProcess, &dataBuf,    &rs, MEM_RELEASE); }
    return 0;
}

namespace {

constexpr ULONG kSecImage = 0x01000000;
constexpr size_t kBootstrapSize = 128;

using NtCreateSectionFn = NTSTATUS (NTAPI*)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES,
                                             PLARGE_INTEGER, ULONG, ULONG, HANDLE);
using NtMapViewOfSectionFn = NTSTATUS (NTAPI*)(HANDLE, HANDLE, PVOID*, ULONG_PTR, SIZE_T,
                                                PLARGE_INTEGER, PSIZE_T, ULONG, ULONG, ULONG);
using NtUnmapViewOfSectionFn = NTSTATUS (NTAPI*)(HANDLE, PVOID);

struct RemoteImageCandidate {
    HMODULE base = nullptr;
    DWORD size = 0;
    std::wstring name;
    std::wstring path;
    int preference = 1000;
};

static bool IsValidPe64(const std::vector<uint8_t>& raw,
                        const IMAGE_NT_HEADERS64*& nt,
                        const IMAGE_SECTION_HEADER*& sections)
{
    nt = nullptr;
    sections = nullptr;
    if (raw.size() < sizeof(IMAGE_DOS_HEADER))
        return false;

    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(raw.data());
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0)
        return false;
    const size_t ntOffset = static_cast<size_t>(dos->e_lfanew);
    if (ntOffset > raw.size() || raw.size() - ntOffset < sizeof(IMAGE_NT_HEADERS64))
        return false;

    nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(raw.data() + ntOffset);
    if (nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
        nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
        nt->OptionalHeader.SizeOfImage == 0 ||
        nt->OptionalHeader.SizeOfImage > 512 * 1024 * 1024)
        return false;

    const size_t sectionOffset = ntOffset + FIELD_OFFSET(IMAGE_NT_HEADERS64, OptionalHeader) +
                                 nt->FileHeader.SizeOfOptionalHeader;
    const size_t sectionBytes = static_cast<size_t>(nt->FileHeader.NumberOfSections) *
                                sizeof(IMAGE_SECTION_HEADER);
    if (nt->FileHeader.NumberOfSections == 0 || sectionOffset > raw.size() ||
        sectionBytes > raw.size() - sectionOffset)
        return false;

    sections = reinterpret_cast<const IMAGE_SECTION_HEADER*>(raw.data() + sectionOffset);
    return true;
}

static bool ExpandImage(const std::vector<uint8_t>& raw, std::vector<uint8_t>& image)
{
    const IMAGE_NT_HEADERS64* nt = nullptr;
    const IMAGE_SECTION_HEADER* sections = nullptr;
    if (!IsValidPe64(raw, nt, sections)) {
        printf("[IBM] Payload is not a valid x64 PE image\n");
        return false;
    }

    image.assign(nt->OptionalHeader.SizeOfImage, 0);
    const size_t headerBytes = (std::min)(static_cast<size_t>(nt->OptionalHeader.SizeOfHeaders),
                                         raw.size());
    if (headerBytes > image.size())
        return false;
    memcpy(image.data(), raw.data(), headerBytes);

    for (WORD index = 0; index < nt->FileHeader.NumberOfSections; ++index) {
        const auto& section = sections[index];
        if (!section.SizeOfRawData)
            continue;
        const size_t rawOffset = section.PointerToRawData;
        const size_t virtualOffset = section.VirtualAddress;
        if (rawOffset > raw.size() || section.SizeOfRawData > raw.size() - rawOffset ||
            virtualOffset > image.size()) {
            printf("[IBM] Payload section %u is outside the PE image\n", index);
            return false;
        }
        const size_t copyBytes = (std::min)(static_cast<size_t>(section.SizeOfRawData),
                                            image.size() - virtualOffset);
        memcpy(image.data() + virtualOffset, raw.data() + rawOffset, copyBytes);
    }
    return true;
}

static bool IsBlacklistedImageName(const wchar_t* name)
{
    static const wchar_t* const blacklist[] = {
        L"ntdll.dll", L"kernel32.dll", L"kernelbase.dll", L"win32u.dll",
        L"ucrtbase.dll", L"rpcrt4.dll", L"combase.dll", L"user32.dll",
        L"gdi32.dll", L"gdi32full.dll", L"msvcp_win.dll", L"imm32.dll",
        L"verifier.dll", L"bcryptprimitives.dll", L"sechost.dll"
    };
    if (_wcsnicmp(name, L"Roblox", 6) == 0 || _wcsnicmp(name, L"noobhook", 8) == 0)
        return true;
    for (const wchar_t* blocked : blacklist)
        if (_wcsicmp(name, blocked) == 0)
            return true;
    return false;
}

static int ImagePreference(const wchar_t* name)
{
    static const wchar_t* const preferred[] = {
        L"shell32.dll", L"setupapi.dll", L"wininet.dll", L"comctl32.dll",
        L"d3d11.dll", L"dbghelp.dll", L"msvcrt.dll", L"shlwapi.dll",
        L"oleaut32.dll", L"gdiplus.dll", L"ole32.dll"
    };
    for (size_t index = 0; index < _countof(preferred); ++index)
        if (_wcsicmp(name, preferred[index]) == 0)
            return static_cast<int>(index);
    return 1000;
}

static bool FindBackingImage(HANDLE hProcess, SIZE_T minimumSize,
                             RemoteImageCandidate& selected)
{
    DWORD bytesNeeded = 0;
    std::vector<HMODULE> modules(256);
    for (;;) {
        if (!EnumProcessModulesEx(hProcess, modules.data(),
                                  static_cast<DWORD>(modules.size() * sizeof(HMODULE)),
                                  &bytesNeeded, LIST_MODULES_ALL))
            return false;
        if (bytesNeeded <= modules.size() * sizeof(HMODULE))
            break;
        modules.resize(bytesNeeded / sizeof(HMODULE) + 32);
    }

    bool found = false;
    const size_t count = bytesNeeded / sizeof(HMODULE);
    for (size_t index = 1; index < count; ++index) {
        MODULEINFO info = {};
        wchar_t name[MAX_PATH] = {};
        wchar_t path[MAX_PATH] = {};
        if (!GetModuleInformation(hProcess, modules[index], &info, sizeof(info)) ||
            !GetModuleBaseNameW(hProcess, modules[index], name, _countof(name)) ||
            !GetModuleFileNameExW(hProcess, modules[index], path, _countof(path)) ||
            info.SizeOfImage < minimumSize || IsBlacklistedImageName(name))
            continue;

        const int preference = ImagePreference(name);
        if (!found || preference < selected.preference ||
            (preference == selected.preference && info.SizeOfImage > selected.size)) {
            selected.base = modules[index];
            selected.size = info.SizeOfImage;
            selected.name = name;
            selected.path = path;
            selected.preference = preference;
            found = true;
        }
    }
    return found;
}

static bool UseEarlyBackingImage(const wchar_t* path, SIZE_T minimumSize,
                                 RemoteImageCandidate& selected)
{
    if (!path || !*path)
        return false;

    HANDLE file = CreateFileW(path, GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return false;

    IMAGE_DOS_HEADER dos = {};
    DWORD transferred = 0;
    bool valid = ReadFile(file, &dos, sizeof(dos), &transferred, nullptr) &&
                 transferred == sizeof(dos) && dos.e_magic == IMAGE_DOS_SIGNATURE &&
                 dos.e_lfanew > 0 && dos.e_lfanew <= 0x100000;
    IMAGE_NT_HEADERS64 nt = {};
    LARGE_INTEGER ntOffset = {};
    ntOffset.QuadPart = dos.e_lfanew;
    if (valid)
        valid = SetFilePointerEx(file, ntOffset, nullptr, FILE_BEGIN) &&
                ReadFile(file, &nt, sizeof(nt), &transferred, nullptr) &&
                transferred == sizeof(nt) && nt.Signature == IMAGE_NT_SIGNATURE &&
                nt.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC &&
                nt.OptionalHeader.SizeOfImage >= minimumSize;
    CloseHandle(file);
    if (!valid)
        return false;

    const wchar_t* name = wcsrchr(path, L'\\');
    selected.base = nullptr;
    selected.size = nt.OptionalHeader.SizeOfImage;
    selected.name = name ? name + 1 : path;
    selected.path = path;
    selected.preference = -1;
    return true;
}

static uintptr_t RemoteAddressOfLocalFunction(HANDLE hProcess, FARPROC function,
                                               std::wstring* ownerNameOut = nullptr)
{
    if (!function)
        return 0;
    HMODULE owner = nullptr;
    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCSTR>(function), &owner))
        return 0;
    wchar_t ownerName[MAX_PATH] = {};
    if (!GetModuleBaseNameW(GetCurrentProcess(), owner, ownerName, _countof(ownerName)))
        return 0;
    if (ownerNameOut)
        *ownerNameOut = ownerName;
    const uintptr_t remoteOwner = GetRemoteModuleBase(hProcess, ownerName);
    if (!remoteOwner)
        return 0;
    return remoteOwner + (reinterpret_cast<uintptr_t>(function) -
                          reinterpret_cast<uintptr_t>(owner));
}

static bool ApplyImageFixups(HANDLE hProcess, std::vector<uint8_t>& image,
                             uintptr_t remoteBase)
{
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(image.data());
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(image.data() + dos->e_lfanew);
    const intptr_t delta = static_cast<intptr_t>(remoteBase - nt->OptionalHeader.ImageBase);

    const auto relocDirectory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    if (delta && relocDirectory.VirtualAddress && relocDirectory.Size) {
        size_t offset = 0;
        while (offset < relocDirectory.Size) {
            const size_t blockRva = static_cast<size_t>(relocDirectory.VirtualAddress) + offset;
            if (blockRva > image.size() || image.size() - blockRva < sizeof(IMAGE_BASE_RELOCATION))
                return false;
            auto* block = reinterpret_cast<IMAGE_BASE_RELOCATION*>(image.data() + blockRva);
            if (!block->SizeOfBlock)
                break;
            if (block->SizeOfBlock < sizeof(IMAGE_BASE_RELOCATION) ||
                block->SizeOfBlock > relocDirectory.Size - offset ||
                blockRva + block->SizeOfBlock > image.size())
                return false;
            const size_t entryCount = (block->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) /
                                      sizeof(WORD);
            const auto* entries = reinterpret_cast<const WORD*>(block + 1);
            for (size_t index = 0; index < entryCount; ++index) {
                if ((entries[index] >> 12) != IMAGE_REL_BASED_DIR64)
                    continue;
                const size_t targetRva = static_cast<size_t>(block->VirtualAddress) +
                                         (entries[index] & 0x0FFF);
                if (targetRva > image.size() || image.size() - targetRva < sizeof(uint64_t))
                    return false;
                auto* value = reinterpret_cast<uint64_t*>(image.data() + targetRva);
                *value += delta;
            }
            offset += block->SizeOfBlock;
        }
        printf("[IBM] Relocations applied (delta 0x%llX)\n",
               static_cast<unsigned long long>(delta));
    }

    const auto importDirectory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    size_t importedModules = 0;
    size_t importedFunctions = 0;
    if (importDirectory.VirtualAddress && importDirectory.Size) {
        const size_t descriptorLimit = importDirectory.Size / sizeof(IMAGE_IMPORT_DESCRIPTOR) + 1;
        for (size_t descriptorIndex = 0; descriptorIndex < descriptorLimit; ++descriptorIndex) {
            const size_t descriptorRva = static_cast<size_t>(importDirectory.VirtualAddress) +
                                         descriptorIndex * sizeof(IMAGE_IMPORT_DESCRIPTOR);
            if (descriptorRva > image.size() ||
                image.size() - descriptorRva < sizeof(IMAGE_IMPORT_DESCRIPTOR))
                return false;
            auto* descriptor = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(image.data() + descriptorRva);
            if (!descriptor->Name && !descriptor->FirstThunk)
                break;
            if (!descriptor->Name || descriptor->Name >= image.size() ||
                !descriptor->FirstThunk || descriptor->FirstThunk >= image.size())
                return false;

            const char* moduleName = reinterpret_cast<const char*>(image.data() + descriptor->Name);
            const size_t remainingName = image.size() - descriptor->Name;
            if (!memchr(moduleName, '\0', remainingName))
                return false;
            HMODULE localModule = GetModuleHandleA(moduleName);
            if (!localModule)
                localModule = LoadLibraryA(moduleName);
            if (!localModule) {
                printf("[IBM] Import module %s could not be loaded in the injector\n", moduleName);
                return false;
            }

            const DWORD lookupRva = descriptor->OriginalFirstThunk
                ? descriptor->OriginalFirstThunk : descriptor->FirstThunk;
            for (size_t thunkIndex = 0; thunkIndex < 65536; ++thunkIndex) {
                const size_t lookupOffset = static_cast<size_t>(lookupRva) +
                                            thunkIndex * sizeof(IMAGE_THUNK_DATA64);
                const size_t iatOffset = static_cast<size_t>(descriptor->FirstThunk) +
                                         thunkIndex * sizeof(IMAGE_THUNK_DATA64);
                if (lookupOffset > image.size() || image.size() - lookupOffset < sizeof(IMAGE_THUNK_DATA64) ||
                    iatOffset > image.size() || image.size() - iatOffset < sizeof(IMAGE_THUNK_DATA64))
                    return false;
                const auto* lookup = reinterpret_cast<const IMAGE_THUNK_DATA64*>(image.data() + lookupOffset);
                if (!lookup->u1.AddressOfData)
                    break;

                FARPROC localFunction = nullptr;
                char functionLabel[256] = {};
                if (IMAGE_SNAP_BY_ORDINAL64(lookup->u1.Ordinal)) {
                    sprintf_s(functionLabel, "ordinal %llu",
                              static_cast<unsigned long long>(IMAGE_ORDINAL64(lookup->u1.Ordinal)));
                    localFunction = GetProcAddress(
                        localModule, MAKEINTRESOURCEA(static_cast<WORD>(IMAGE_ORDINAL64(lookup->u1.Ordinal))));
                } else {
                    const size_t nameRva = static_cast<size_t>(lookup->u1.AddressOfData);
                    if (nameRva > image.size() ||
                        image.size() - nameRva < sizeof(IMAGE_IMPORT_BY_NAME))
                        return false;
                    const auto* importName = reinterpret_cast<const IMAGE_IMPORT_BY_NAME*>(image.data() + nameRva);
                    const char* functionName = reinterpret_cast<const char*>(importName->Name);
                    const size_t remainingFunctionName = image.size() - nameRva - sizeof(WORD);
                    if (!memchr(functionName, '\0', remainingFunctionName))
                        return false;
                    strncpy_s(functionLabel, functionName, _TRUNCATE);
                    localFunction = GetProcAddress(localModule, functionName);
                }
                if (!localFunction) {
                    printf("[IBM] Missing local export %s!%s\n", moduleName, functionLabel);
                    return false;
                }
                std::wstring ownerName;
                const uintptr_t remoteFunction = RemoteAddressOfLocalFunction(
                    hProcess, localFunction, &ownerName);
                if (!remoteFunction) {
                    printf("[IBM] Import %s!%s resolves through %ls, which is not loaded in the Player\n",
                           moduleName, functionLabel,
                           ownerName.empty() ? L"an unknown module" : ownerName.c_str());
                    return false;
                }
                auto* iat = reinterpret_cast<IMAGE_THUNK_DATA64*>(image.data() + iatOffset);
                iat->u1.Function = remoteFunction;
                ++importedFunctions;
            }
            ++importedModules;
        }
    }
    printf("[IBM] IAT resolved externally: %zu modules, %zu functions\n",
           importedModules, importedFunctions);
    return true;
}

static DWORD ProtectionForSection(const IMAGE_SECTION_HEADER& section)
{
    const bool execute = (section.Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
    const bool read = (section.Characteristics & IMAGE_SCN_MEM_READ) != 0;
    const bool write = (section.Characteristics & IMAGE_SCN_MEM_WRITE) != 0;
    if (execute && write) return PAGE_EXECUTE_READWRITE;
    if (execute && read) return PAGE_EXECUTE_READ;
    if (execute) return PAGE_EXECUTE;
    if (read && write) return PAGE_READWRITE;
    if (write) return PAGE_READWRITE;
    if (read) return PAGE_READONLY;
    return PAGE_NOACCESS;
}

static bool SetImageProtections(HANDLE hProcess, uintptr_t remoteBase,
                                const std::vector<uint8_t>& image)
{
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(image.data());
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(image.data() + dos->e_lfanew);
    const auto* sections = IMAGE_FIRST_SECTION(nt);
    bool okay = true;

    void* headerBase = reinterpret_cast<void*>(remoteBase);
    SIZE_T headerSize = nt->OptionalHeader.SizeOfHeaders;
    ULONG oldProtection = 0;
    NTSTATUS status = SysNtProtectVirtualMemory(hProcess, &headerBase, &headerSize,
                                                 PAGE_READONLY, &oldProtection);
    if (status < 0) {
        printf("[IBM] Header protection failed: 0x%08lX\n", static_cast<unsigned long>(status));
        okay = false;
    }

    for (WORD index = 0; index < nt->FileHeader.NumberOfSections; ++index) {
        const SIZE_T sectionBytes = (std::max)(static_cast<SIZE_T>(sections[index].Misc.VirtualSize),
                                               static_cast<SIZE_T>(sections[index].SizeOfRawData));
        if (!sectionBytes)
            continue;
        void* sectionBase = reinterpret_cast<void*>(remoteBase + sections[index].VirtualAddress);
        SIZE_T sectionSize = sectionBytes;
        const DWORD protection = ProtectionForSection(sections[index]);
        status = SysNtProtectVirtualMemory(hProcess, &sectionBase, &sectionSize,
                                            protection, &oldProtection);
        char sectionName[9] = {};
        memcpy(sectionName, sections[index].Name, 8);
        if (status < 0) {
            printf("[IBM] Protection for %.8s failed: 0x%08lX\n", sectionName,
                   static_cast<unsigned long>(status));
            okay = false;
        } else {
            printf("[IBM] %.8s -> 0x%02lX\n", sectionName,
                   static_cast<unsigned long>(protection));
        }
    }
    FlushInstructionCache(hProcess, reinterpret_cast<void*>(remoteBase), image.size());
    return okay;
}

static bool RemoteTextSection(HANDLE hProcess, HMODULE module,
                              uintptr_t& textBase, SIZE_T& textSize)
{
    IMAGE_DOS_HEADER dos = {};
    if (!ReadProcessMemory(hProcess, module, &dos, sizeof(dos), nullptr) ||
        dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew <= 0 || dos.e_lfanew > 0x100000)
        return false;
    IMAGE_NT_HEADERS64 nt = {};
    const uintptr_t ntAddress = reinterpret_cast<uintptr_t>(module) + dos.e_lfanew;
    if (!ReadProcessMemory(hProcess, reinterpret_cast<void*>(ntAddress), &nt, sizeof(nt), nullptr) ||
        nt.Signature != IMAGE_NT_SIGNATURE || nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        return false;

    const uintptr_t sectionAddress = ntAddress + FIELD_OFFSET(IMAGE_NT_HEADERS64, OptionalHeader) +
                                     nt.FileHeader.SizeOfOptionalHeader;
    for (WORD index = 0; index < nt.FileHeader.NumberOfSections; ++index) {
        IMAGE_SECTION_HEADER section = {};
        if (!ReadProcessMemory(hProcess,
                               reinterpret_cast<void*>(sectionAddress + index * sizeof(section)),
                               &section, sizeof(section), nullptr))
            return false;
        char name[9] = {};
        memcpy(name, section.Name, 8);
        if (strcmp(name, ".text") == 0) {
            textBase = reinterpret_cast<uintptr_t>(module) + section.VirtualAddress;
            textSize = section.Misc.VirtualSize;
            return textSize != 0;
        }
    }
    return false;
}

static bool FindStompAddress(HANDLE hProcess, HMODULE backingModule,
                             uintptr_t& stompAddress, std::wstring& stompName)
{
    static const wchar_t* const preferred[] = {
        L"dbghelp.dll", L"msvcrt.dll", L"comctl32.dll", L"comdlg32.dll",
        L"wininet.dll", L"d3d11.dll", L"dxgi.dll", L"ole32.dll",
        L"shell32.dll", L"setupapi.dll", L"shlwapi.dll"
    };

    DWORD bytesNeeded = 0;
    std::vector<HMODULE> modules(256);
    for (;;) {
        if (!EnumProcessModulesEx(hProcess, modules.data(),
                                  static_cast<DWORD>(modules.size() * sizeof(HMODULE)),
                                  &bytesNeeded, LIST_MODULES_ALL))
            return false;
        if (bytesNeeded <= modules.size() * sizeof(HMODULE))
            break;
        modules.resize(bytesNeeded / sizeof(HMODULE) + 32);
    }

    HMODULE selectedModule = nullptr;
    uintptr_t selectedText = 0;
    SIZE_T selectedSize = 0;
    int selectedRank = 1000;
    const size_t count = bytesNeeded / sizeof(HMODULE);
    for (size_t index = 1; index < count; ++index) {
        if (modules[index] == backingModule)
            continue;
        wchar_t name[MAX_PATH] = {};
        if (!GetModuleBaseNameW(hProcess, modules[index], name, _countof(name)) ||
            IsBlacklistedImageName(name))
            continue;
        uintptr_t textBase = 0;
        SIZE_T textSize = 0;
        if (!RemoteTextSection(hProcess, modules[index], textBase, textSize) ||
            textSize < kBootstrapSize * 4)
            continue;
        int rank = 1000;
        for (size_t candidate = 0; candidate < _countof(preferred); ++candidate)
            if (_wcsicmp(name, preferred[candidate]) == 0) {
                rank = static_cast<int>(candidate);
                break;
            }
        if (!selectedModule || rank < selectedRank) {
            selectedModule = modules[index];
            selectedText = textBase;
            selectedSize = textSize;
            selectedRank = rank;
            stompName = name;
        }
    }
    if (!selectedModule)
        return false;

    // Prefer an aligned padding run.  If this DLL has no sufficiently large
    // cave, use the same late-.text window as the tested reference injector.
    std::vector<uint8_t> textBytes(selectedSize);
    SIZE_T read = 0;
    if (ReadProcessMemory(hProcess, reinterpret_cast<void*>(selectedText), textBytes.data(),
                          textBytes.size(), &read) && read == textBytes.size()) {
        const size_t first = (textBytes.size() / 2 + 15) & ~static_cast<size_t>(15);
        for (size_t offset = first; offset + kBootstrapSize <= textBytes.size(); offset += 16) {
            bool padding = true;
            for (size_t byte = 0; byte < kBootstrapSize; ++byte) {
                const uint8_t value = textBytes[offset + byte];
                if (value != 0x00 && value != 0x90 && value != 0xCC) {
                    padding = false;
                    break;
                }
            }
            if (padding) {
                stompAddress = selectedText + offset;
                printf("[IBM] Using aligned padding in %ls .text at 0x%llX\n",
                       stompName.c_str(), static_cast<unsigned long long>(stompAddress));
                return true;
            }
        }
    }

    size_t offset = (selectedSize * 7) / 8;
    offset &= ~static_cast<size_t>(15);
    if (offset + kBootstrapSize > selectedSize)
        return false;
    stompAddress = selectedText + offset;
    printf("[IBM] No padding cave found; using tested late-.text position in %ls at 0x%llX\n",
           stompName.c_str(), static_cast<unsigned long long>(stompAddress));
    return true;
}

struct SystemHandleEntry728 {
    USHORT processId;
    USHORT creatorBackTraceIndex;
    UCHAR objectTypeIndex;
    UCHAR handleAttributes;
    USHORT handleValue;
    PVOID object;
    ACCESS_MASK grantedAccess;
};

struct SystemHandleInfo728 {
    ULONG count;
    SystemHandleEntry728 handles[1];
};

static std::vector<HANDLE> FindIoCompletionHandles728(HANDLE hProcess, DWORD processId)
{
    constexpr ULONG kSystemHandleInformation = 16;
    std::vector<uint8_t> buffer(4 * 1024 * 1024);
    ULONG required = 0;
    NTSTATUS status = 0;
    for (int attempt = 0; attempt < 5; ++attempt) {
        status = SysNtQuerySystemInformation(kSystemHandleInformation, buffer.data(),
                                              static_cast<ULONG>(buffer.size()), &required);
        if (status != static_cast<NTSTATUS>(0xC0000004L))
            break;
        buffer.resize(static_cast<size_t>(required) + 1024 * 1024);
    }
    std::vector<HANDLE> result;
    if (status < 0)
        return result;

    const auto* info = reinterpret_cast<const SystemHandleInfo728*>(buffer.data());
    for (ULONG index = 0; index < info->count && result.size() < 8; ++index) {
        const auto& entry = info->handles[index];
        if (entry.processId != static_cast<USHORT>(processId))
            continue;
        HANDLE duplicate = nullptr;
        status = SysNtDuplicateObject(hProcess,
                                       reinterpret_cast<HANDLE>(static_cast<uintptr_t>(entry.handleValue)),
                                       GetCurrentProcess(), &duplicate, 0, 0,
                                       DUPLICATE_SAME_ACCESS);
        if (status < 0 || !duplicate)
            continue;
        alignas(16) uint8_t typeBuffer[1024] = {};
        ULONG typeBytes = 0;
        status = SysNtQueryObject(duplicate, 2, typeBuffer, sizeof(typeBuffer), &typeBytes);
        if (status >= 0) {
            const auto* typeName = reinterpret_cast<const UNICODE_STRING*>(typeBuffer);
            if (typeName->Buffer && typeName->Length == 12 * sizeof(wchar_t) &&
                wcsncmp(typeName->Buffer, L"IoCompletion", 12) == 0) {
                result.push_back(duplicate);
                duplicate = nullptr;
            }
        }
        if (duplicate)
            CloseHandle(duplicate);
    }
    return result;
}

static bool BuildEntryBootstrap(std::array<uint8_t, kBootstrapSize>& code,
                                uintptr_t imageBase, uintptr_t entryPoint,
                                uintptr_t flagAddress, uintptr_t rtlAddFunctionTable,
                                uintptr_t exceptionTable, DWORD exceptionCount)
{
    size_t offset = 0;
    auto emitByte = [&](uint8_t value) {
        if (offset >= code.size()) return false;
        code[offset++] = value;
        return true;
    };
    auto emitBytes = [&](std::initializer_list<uint8_t> values) {
        for (uint8_t value : values)
            if (!emitByte(value)) return false;
        return true;
    };
    auto emit32 = [&](uint32_t value) {
        if (offset + sizeof(value) > code.size()) return false;
        memcpy(code.data() + offset, &value, sizeof(value));
        offset += sizeof(value);
        return true;
    };
    auto emit64 = [&](uint64_t value) {
        if (offset + sizeof(value) > code.size()) return false;
        memcpy(code.data() + offset, &value, sizeof(value));
        offset += sizeof(value);
        return true;
    };

    // Preserve every nonvolatile general-purpose register and align the stack.
    if (!emitBytes({0x55, 0x53, 0x56, 0x57, 0x41, 0x54, 0x41, 0x55,
                    0x41, 0x56, 0x41, 0x57, 0x48, 0x83, 0xEC, 0x28}))
        return false;

    if (rtlAddFunctionTable && exceptionTable && exceptionCount) {
        if (!emitBytes({0x48, 0xB9}) || !emit64(exceptionTable) ||
            !emitByte(0xBA) || !emit32(exceptionCount) ||
            !emitBytes({0x49, 0xB8}) || !emit64(imageBase) ||
            !emitBytes({0x48, 0xB8}) || !emit64(rtlAddFunctionTable) ||
            !emitBytes({0xFF, 0xD0}))
            return false;
    }

    // DllMain(imageBase, DLL_PROCESS_ATTACH, nullptr).
    if (!emitBytes({0x48, 0xB9}) || !emit64(imageBase) ||
        !emitByte(0xBA) || !emit32(DLL_PROCESS_ATTACH) ||
        !emitBytes({0x45, 0x31, 0xC0}) ||
        !emitBytes({0x48, 0xB8}) || !emit64(entryPoint) ||
        !emitBytes({0xFF, 0xD0}) ||
        !emitBytes({0x48, 0xB8}) || !emit64(flagAddress) ||
        !emitBytes({0xC7, 0x00}) || !emit32(1) ||
        !emitBytes({0x48, 0x83, 0xC4, 0x28, 0x41, 0x5F, 0x41, 0x5E,
                    0x41, 0x5D, 0x41, 0x5C, 0x5F, 0x5E, 0x5B, 0x5D, 0xC3}))
        return false;
    return true;
}

static bool BuildLoadLibraryBootstrap(std::array<uint8_t, kBootstrapSize>& code,
                                      uintptr_t loadLibraryW, uintptr_t moduleName,
                                      uintptr_t resultAddress)
{
    size_t offset = 0;
    auto emitBytes = [&](std::initializer_list<uint8_t> values) {
        if (offset + values.size() > code.size()) return false;
        for (uint8_t value : values) code[offset++] = value;
        return true;
    };
    auto emit64 = [&](uint64_t value) {
        if (offset + sizeof(value) > code.size()) return false;
        memcpy(code.data() + offset, &value, sizeof(value));
        offset += sizeof(value);
        return true;
    };

    // Preserve nonvolatile registers, call LoadLibraryW(name), then publish
    // the returned HMODULE only after the loader call has completely returned.
    return emitBytes({0x55, 0x53, 0x56, 0x57, 0x41, 0x54, 0x41, 0x55,
                      0x41, 0x56, 0x41, 0x57, 0x48, 0x83, 0xEC, 0x28}) &&
           emitBytes({0x48, 0xB9}) && emit64(moduleName) &&
           emitBytes({0x48, 0xB8}) && emit64(loadLibraryW) &&
           emitBytes({0xFF, 0xD0}) &&
           emitBytes({0x49, 0xBA}) && emit64(resultAddress) &&
           emitBytes({0x49, 0x89, 0x02}) &&
           emitBytes({0x48, 0x83, 0xC4, 0x28, 0x41, 0x5F, 0x41, 0x5E,
                      0x41, 0x5D, 0x41, 0x5C, 0x5F, 0x5E, 0x5B, 0x5D, 0xC3});
}

struct TpTaskCallbacks728 {
    void* execute;
    void* unposted;
};

struct TpTask728 {
    TpTaskCallbacks728* callbacks;
    UINT32 numaNode;
    UINT8 idealProcessor;
    char padding[3];
    LIST_ENTRY listEntry;
};

struct TpDirect728 {
    TpTask728 task;
    UINT64 lock;
    LIST_ENTRY ioCompletionInformationList;
    void* callback;
    UINT32 numaNode;
    UINT8 idealProcessor;
    char padding[3];
};

static uintptr_t LoadRemoteModuleThroughIoCompletion(HANDLE hProcess, HMODULE backingModule,
                                                       const wchar_t* moduleName)
{
    if (uintptr_t loaded = GetRemoteModuleBase(hProcess, moduleName))
        return loaded;

    const FARPROC localLoadLibraryW = GetProcAddress(GetModuleHandleW(L"kernel32.dll"),
                                                      "LoadLibraryW");
    const uintptr_t remoteLoadLibraryW = RemoteAddressOfLocalFunction(hProcess,
                                                                       localLoadLibraryW);
    if (!remoteLoadLibraryW) {
        printf("[IBM] Cannot resolve LoadLibraryW inside the Player\n");
        return 0;
    }

    uintptr_t stompAddress = 0;
    std::wstring stompName;
    if (!FindStompAddress(hProcess, backingModule, stompAddress, stompName)) {
        printf("[IBM] Cannot find image code for the dependency-loader callback\n");
        return 0;
    }

    void* workBuffer = nullptr;
    SIZE_T workSize = 0x1000;
    NTSTATUS status = SysNtAllocateVirtualMemory(hProcess, &workBuffer, 0, &workSize,
                                                  MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (status < 0 || !workBuffer) {
        printf("[IBM] Dependency-loader work allocation failed: 0x%08lX\n",
               static_cast<unsigned long>(status));
        return 0;
    }

    const uintptr_t resultAddress = reinterpret_cast<uintptr_t>(workBuffer);
    const uintptr_t nameAddress = resultAddress + 16;
    const uintptr_t callbacksAddress = resultAddress + 256;
    const uintptr_t directAddress = resultAddress + 288;
    const size_t nameBytes = (wcslen(moduleName) + 1) * sizeof(wchar_t);
    SIZE_T transferred = 0;
    status = SysNtWriteVirtualMemory(hProcess, reinterpret_cast<void*>(nameAddress),
                                      moduleName, nameBytes, &transferred);
    if (status < 0 || transferred != nameBytes) {
        printf("[IBM] Could not write dependency name %ls\n", moduleName);
        return 0;
    }

    std::array<uint8_t, kBootstrapSize> bootstrap = {};
    if (!BuildLoadLibraryBootstrap(bootstrap, remoteLoadLibraryW, nameAddress,
                                    resultAddress))
        return 0;
    std::array<uint8_t, kBootstrapSize> originalCode = {};
    SIZE_T read = 0;
    if (!ReadProcessMemory(hProcess, reinterpret_cast<void*>(stompAddress),
                           originalCode.data(), originalCode.size(), &read) ||
        read != originalCode.size()) {
        printf("[IBM] Cannot save the dependency-loader callback bytes\n");
        return 0;
    }

    void* protectBase = reinterpret_cast<void*>(stompAddress);
    SIZE_T protectSize = bootstrap.size();
    ULONG originalProtection = 0;
    status = SysNtProtectVirtualMemory(hProcess, &protectBase, &protectSize,
                                        PAGE_READWRITE, &originalProtection);
    if (status < 0) {
        printf("[IBM] Cannot open dependency-loader callback for writing: 0x%08lX\n",
               static_cast<unsigned long>(status));
        return 0;
    }
    status = SysNtWriteVirtualMemory(hProcess, reinterpret_cast<void*>(stompAddress),
                                      bootstrap.data(), bootstrap.size(), &transferred);
    protectBase = reinterpret_cast<void*>(stompAddress);
    protectSize = bootstrap.size();
    ULONG ignoredProtection = 0;
    SysNtProtectVirtualMemory(hProcess, &protectBase, &protectSize,
                              PAGE_EXECUTE_READ, &ignoredProtection);
    FlushInstructionCache(hProcess, reinterpret_cast<void*>(stompAddress), bootstrap.size());
    if (status < 0 || transferred != bootstrap.size()) {
        printf("[IBM] Could not install dependency-loader callback: 0x%08lX\n",
               static_cast<unsigned long>(status));
        return 0;
    }

    TpTaskCallbacks728 callbacks = { reinterpret_cast<void*>(stompAddress), nullptr };
    TpDirect728 direct = {};
    direct.task.callbacks = reinterpret_cast<TpTaskCallbacks728*>(callbacksAddress);
    direct.callback = reinterpret_cast<void*>(stompAddress);
    status = SysNtWriteVirtualMemory(hProcess, reinterpret_cast<void*>(callbacksAddress),
                                      &callbacks, sizeof(callbacks), &transferred);
    if (status >= 0)
        status = SysNtWriteVirtualMemory(hProcess, reinterpret_cast<void*>(directAddress),
                                          &direct, sizeof(direct), &transferred);

    uintptr_t result = 0;
    if (status >= 0) {
        auto completionHandles = FindIoCompletionHandles728(hProcess, GetProcessId(hProcess));
        printf("[IBM] Loading %ls through %zu IoCompletion handle(s)\n",
               moduleName, completionHandles.size());
        for (HANDLE completion : completionHandles) {
            status = SysNtSetIoCompletion(completion, directAddress, nullptr, 0, 0);
            printf("[IBM] Dependency NtSetIoCompletion: 0x%08lX\n",
                   static_cast<unsigned long>(status));
            if (status < 0)
                continue;
            for (int wait = 0; wait < 200; ++wait) {
                SIZE_T resultBytes = 0;
                SysNtReadVirtualMemory(hProcess, reinterpret_cast<void*>(resultAddress),
                                        &result, sizeof(result), &resultBytes);
                if (result)
                    break;
                if (WaitForSingleObject(hProcess, 0) == WAIT_OBJECT_0)
                    break;
                Sleep(50);
            }
            break;
        }
        for (HANDLE completion : completionHandles)
            CloseHandle(completion);
    }

    if (result)
        Sleep(10);
    protectBase = reinterpret_cast<void*>(stompAddress);
    protectSize = bootstrap.size();
    status = SysNtProtectVirtualMemory(hProcess, &protectBase, &protectSize,
                                        PAGE_READWRITE, &ignoredProtection);
    if (status >= 0) {
        SysNtWriteVirtualMemory(hProcess, reinterpret_cast<void*>(stompAddress),
                                originalCode.data(), originalCode.size(), &transferred);
        protectBase = reinterpret_cast<void*>(stompAddress);
        protectSize = bootstrap.size();
        SysNtProtectVirtualMemory(hProcess, &protectBase, &protectSize,
                                  originalProtection, &ignoredProtection);
        FlushInstructionCache(hProcess, reinterpret_cast<void*>(stompAddress), bootstrap.size());
        printf("[IBM] Restored the %ls dependency-loader callback\n", stompName.c_str());
    }

    const uintptr_t moduleBase = GetRemoteModuleBase(hProcess, moduleName);
    if (!result || !moduleBase) {
        printf("[IBM] LoadLibraryW did not load %ls (result 0x%llX)\n",
               moduleName, static_cast<unsigned long long>(result));
        return 0;
    }
    printf("[IBM] Loaded %ls at 0x%llX\n", moduleName,
           static_cast<unsigned long long>(moduleBase));
    return moduleBase;
}

static bool LoadMissingImportModules(HANDLE hProcess, HMODULE backingModule,
                                     const std::vector<uint8_t>& image)
{
    if (image.size() < sizeof(IMAGE_DOS_HEADER))
        return false;
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(image.data());
    if (dos->e_lfanew <= 0 || static_cast<size_t>(dos->e_lfanew) > image.size() ||
        image.size() - static_cast<size_t>(dos->e_lfanew) < sizeof(IMAGE_NT_HEADERS64))
        return false;
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(image.data() + dos->e_lfanew);
    const auto imports = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!imports.VirtualAddress || !imports.Size)
        return true;

    size_t checked = 0;
    size_t loaded = 0;
    const size_t descriptorLimit = imports.Size / sizeof(IMAGE_IMPORT_DESCRIPTOR) + 1;
    for (size_t index = 0; index < descriptorLimit; ++index) {
        const size_t descriptorRva = static_cast<size_t>(imports.VirtualAddress) +
                                     index * sizeof(IMAGE_IMPORT_DESCRIPTOR);
        if (descriptorRva > image.size() ||
            image.size() - descriptorRva < sizeof(IMAGE_IMPORT_DESCRIPTOR))
            return false;
        const auto* descriptor = reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR*>(
            image.data() + descriptorRva);
        if (!descriptor->Name && !descriptor->FirstThunk)
            break;
        if (!descriptor->Name || descriptor->Name >= image.size())
            return false;
        const char* moduleName = reinterpret_cast<const char*>(image.data() + descriptor->Name);
        const size_t remaining = image.size() - descriptor->Name;
        const void* terminator = memchr(moduleName, '\0', remaining);
        if (!terminator)
            return false;
        const size_t moduleLength = static_cast<const char*>(terminator) - moduleName;
        if (!moduleLength || moduleLength >= MAX_PATH)
            return false;

        wchar_t wideName[MAX_PATH] = {};
        const int converted = MultiByteToWideChar(CP_ACP, 0,
                                                   moduleName, static_cast<int>(moduleLength),
                                                   wideName, _countof(wideName) - 1);
        if (converted <= 0)
            return false;
        wideName[converted] = L'\0';
        ++checked;

        if (GetRemoteModuleBase(hProcess, wideName)) {
            printf("[IBM] Import dependency already loaded: %ls\n", wideName);
            continue;
        }
        printf("[IBM] Import dependency missing: %ls\n", wideName);
        if (!LoadRemoteModuleThroughIoCompletion(hProcess, backingModule, wideName))
            return false;
        ++loaded;
    }
    printf("[IBM] Import dependencies ready: %zu checked, %zu loaded\n", checked, loaded);
    return true;
}

} // namespace

uintptr_t GetImageExportRva(const std::vector<uint8_t>& dllData, const char* exportName)
{
    if (!exportName)
        return 0;
    std::vector<uint8_t> image;
    if (!ExpandImage(dllData, image))
        return 0;
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(image.data());
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(image.data() + dos->e_lfanew);
    const IMAGE_DATA_DIRECTORY exports =
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (!exports.VirtualAddress || exports.VirtualAddress >= image.size() ||
        image.size() - exports.VirtualAddress < sizeof(IMAGE_EXPORT_DIRECTORY))
        return 0;

    const auto* directory = reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(
        image.data() + exports.VirtualAddress);
    if (directory->AddressOfNames >= image.size() ||
        directory->AddressOfNameOrdinals >= image.size() ||
        directory->AddressOfFunctions >= image.size())
        return 0;
    const auto* names = reinterpret_cast<const DWORD*>(image.data() + directory->AddressOfNames);
    const auto* ordinals = reinterpret_cast<const WORD*>(image.data() + directory->AddressOfNameOrdinals);
    const auto* functions = reinterpret_cast<const DWORD*>(image.data() + directory->AddressOfFunctions);
    for (DWORD index = 0; index < directory->NumberOfNames; ++index) {
        if (directory->AddressOfNames + (index + 1) * sizeof(DWORD) > image.size() ||
            directory->AddressOfNameOrdinals + (index + 1) * sizeof(WORD) > image.size())
            return 0;
        const DWORD nameRva = names[index];
        if (nameRva >= image.size())
            continue;
        const char* name = reinterpret_cast<const char*>(image.data() + nameRva);
        if (!memchr(name, '\0', image.size() - nameRva))
            continue;
        if (strcmp(name, exportName) != 0)
            continue;
        const WORD ordinal = ordinals[index];
        if (ordinal >= directory->NumberOfFunctions ||
            directory->AddressOfFunctions + (ordinal + 1) * sizeof(DWORD) > image.size())
            return 0;
        return functions[ordinal];
    }
    return 0;
}

uintptr_t ImageBackedManualMapDll(HANDLE hProcess, const std::vector<uint8_t>& dllData,
                                  bool runEntryPoint, const wchar_t* earlyBackingPath)
{
    std::vector<uint8_t> image;
    if (!ExpandImage(dllData, image))
        return 0;
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(image.data());
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(image.data() + dos->e_lfanew);
    const auto tlsDirectory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];
    if (tlsDirectory.VirtualAddress && tlsDirectory.Size) {
        printf("[IBM] Payload has TLS callbacks; refusing to skip loader TLS initialization\n");
        return 0;
    }
    printf("[IBM] Expanded payload to 0x%zX bytes (entry RVA 0x%lX)\n",
           image.size(), static_cast<unsigned long>(nt->OptionalHeader.AddressOfEntryPoint));

    RemoteImageCandidate backing;
    if (earlyBackingPath) {
        if (!UseEarlyBackingImage(earlyBackingPath, image.size(), backing)) {
            printf("[IBM] Early backing image is unavailable or too small: %ls\n",
                   earlyBackingPath);
            return 0;
        }
    } else {
        for (int attempt = 0; attempt < 100; ++attempt) {
            backing = {};
            if (FindBackingImage(hProcess, image.size(), backing))
                break;
            DWORD exitCode = 0;
            if (GetExitCodeProcess(hProcess, &exitCode) && exitCode != STILL_ACTIVE)
                break;
            Sleep(50);
        }
    }
    if (backing.path.empty()) {
        printf("[IBM] No loaded image large enough to back the payload\n");
        return 0;
    }
    printf("[IBM] Backing image: %ls at 0x%p (0x%lX bytes)\n",
           backing.name.c_str(), backing.base, static_cast<unsigned long>(backing.size));
    printf("[IBM] Backing path: %ls\n", backing.path.c_str());

    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    const auto ntCreateSection = reinterpret_cast<NtCreateSectionFn>(
        GetProcAddress(ntdll, "NtCreateSection"));
    const auto ntMapViewOfSection = reinterpret_cast<NtMapViewOfSectionFn>(
        GetProcAddress(ntdll, "NtMapViewOfSection"));
    const auto ntUnmapViewOfSection = reinterpret_cast<NtUnmapViewOfSectionFn>(
        GetProcAddress(ntdll, "NtUnmapViewOfSection"));
    if (!ntCreateSection || !ntMapViewOfSection || !ntUnmapViewOfSection) {
        printf("[IBM] Required ntdll section functions are unavailable\n");
        return 0;
    }

    HANDLE file = CreateFileW(backing.path.c_str(), GENERIC_READ | GENERIC_EXECUTE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        printf("[IBM] Cannot open backing image (error %lu)\n", GetLastError());
        return 0;
    }
    HANDLE section = nullptr;
    NTSTATUS status = ntCreateSection(&section, SECTION_ALL_ACCESS, nullptr, nullptr,
                                       PAGE_READONLY, kSecImage, file);
    CloseHandle(file);
    if (status < 0 || !section) {
        printf("[IBM] NtCreateSection(SEC_IMAGE) failed: 0x%08lX\n",
               static_cast<unsigned long>(status));
        return 0;
    }

    void* remoteView = nullptr;
    SIZE_T viewSize = 0;
    status = ntMapViewOfSection(section, hProcess, &remoteView, 0, 0, nullptr,
                                 &viewSize, 2, 0, PAGE_READONLY);
    CloseHandle(section);
    if (status < 0 || !remoteView) {
        printf("[IBM] NtMapViewOfSection failed: 0x%08lX\n",
               static_cast<unsigned long>(status));
        return 0;
    }
    const uintptr_t remoteBase = reinterpret_cast<uintptr_t>(remoteView);
    printf("[IBM] Secondary SEC_IMAGE view mapped at 0x%llX (0x%zX bytes)\n",
           static_cast<unsigned long long>(remoteBase), viewSize);
    if (viewSize < image.size()) {
        printf("[IBM] Backing view is smaller than the payload\n");
        ntUnmapViewOfSection(hProcess, remoteView);
        return 0;
    }

    // The 0.728 app shell initially omits several DLLs that the game process
    // normally loads later.  Reproduce the loader's dependency stage for the
    // supplied noobHook image before resolving its IAT.  Each missing signed
    // system dependency is loaded through an existing Player thread-pool path;
    // the noobHook payload itself remains manually image-backed.
    if (!LoadMissingImportModules(hProcess, backing.base, image)) {
        printf("[IBM] Could not initialize all payload import dependencies\n");
        ntUnmapViewOfSection(hProcess, remoteView);
        return 0;
    }

    if (!ApplyImageFixups(hProcess, image, remoteBase)) {
        printf("[IBM] Relocation/import fixups failed\n");
        ntUnmapViewOfSection(hProcess, remoteView);
        return 0;
    }

    void* writeBase = remoteView;
    SIZE_T writeSize = image.size();
    ULONG originalProtection = 0;
    status = SysNtProtectVirtualMemory(hProcess, &writeBase, &writeSize,
                                        PAGE_READWRITE, &originalProtection);
    if (status < 0) {
        printf("[IBM] Cannot make the image view writable: 0x%08lX\n",
               static_cast<unsigned long>(status));
        ntUnmapViewOfSection(hProcess, remoteView);
        return 0;
    }
    SIZE_T written = 0;
    status = SysNtWriteVirtualMemory(hProcess, remoteView, image.data(), image.size(), &written);
    if (status < 0 || written != image.size()) {
        printf("[IBM] Payload image write failed: 0x%08lX (%zu/0x%zX bytes)\n",
               static_cast<unsigned long>(status), written, image.size());
        ntUnmapViewOfSection(hProcess, remoteView);
        return 0;
    }
    if (!SetImageProtections(hProcess, remoteBase, image)) {
        printf("[IBM] Could not reproduce all payload section protections\n");
        ntUnmapViewOfSection(hProcess, remoteView);
        return 0;
    }

    MEMORY_BASIC_INFORMATION memory = {};
    if (VirtualQueryEx(hProcess, remoteView, &memory, sizeof(memory)) == sizeof(memory))
        printf("[IBM] Payload view ready: type=0x%lX protect=0x%lX\n",
               static_cast<unsigned long>(memory.Type),
               static_cast<unsigned long>(memory.Protect));
    if (memory.Type != MEM_IMAGE) {
        printf("[IBM] Payload lost MEM_IMAGE backing; aborting\n");
        ntUnmapViewOfSection(hProcess, remoteView);
        return 0;
    }

    if (!runEntryPoint) {
        printf("[IBM] Image-backed payload mapped without running its entry point at 0x%llX\n",
               static_cast<unsigned long long>(remoteBase));
        return remoteBase;
    }

    uintptr_t stompAddress = 0;
    std::wstring stompName;
    if (!FindStompAddress(hProcess, backing.base, stompAddress, stompName)) {
        printf("[IBM] No suitable image-code callback location is loaded\n");
        ntUnmapViewOfSection(hProcess, remoteView);
        return 0;
    }

    void* workBuffer = nullptr;
    SIZE_T workSize = 0x1000;
    status = SysNtAllocateVirtualMemory(hProcess, &workBuffer, 0, &workSize,
                                        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (status < 0 || !workBuffer) {
        printf("[IBM] Work-buffer allocation failed: 0x%08lX\n",
               static_cast<unsigned long>(status));
        ntUnmapViewOfSection(hProcess, remoteView);
        return 0;
    }
    const uintptr_t flagAddress = reinterpret_cast<uintptr_t>(workBuffer);
    const uintptr_t callbacksAddress = flagAddress + 16;
    const uintptr_t directAddress = flagAddress + 48;

    const auto exceptionDirectory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
    const uintptr_t localRtlAddFunctionTable = reinterpret_cast<uintptr_t>(
        GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlAddFunctionTable"));
    const uintptr_t remoteRtlAddFunctionTable = RemoteAddressOfLocalFunction(
        hProcess, reinterpret_cast<FARPROC>(localRtlAddFunctionTable));
    std::array<uint8_t, kBootstrapSize> bootstrap = {};
    if (!BuildEntryBootstrap(
            bootstrap, remoteBase, remoteBase + nt->OptionalHeader.AddressOfEntryPoint,
            flagAddress, remoteRtlAddFunctionTable,
            exceptionDirectory.VirtualAddress ? remoteBase + exceptionDirectory.VirtualAddress : 0,
            exceptionDirectory.Size / sizeof(RUNTIME_FUNCTION))) {
        printf("[IBM] Entry bootstrap exceeded its image-code slot\n");
        ntUnmapViewOfSection(hProcess, remoteView);
        return 0;
    }

    std::array<uint8_t, kBootstrapSize> originalCode = {};
    SIZE_T read = 0;
    if (!ReadProcessMemory(hProcess, reinterpret_cast<void*>(stompAddress), originalCode.data(),
                           originalCode.size(), &read) || read != originalCode.size()) {
        printf("[IBM] Could not save the image-code callback bytes\n");
        ntUnmapViewOfSection(hProcess, remoteView);
        return 0;
    }

    void* protectBase = reinterpret_cast<void*>(stompAddress);
    SIZE_T protectSize = bootstrap.size();
    ULONG stompOriginalProtection = 0;
    status = SysNtProtectVirtualMemory(hProcess, &protectBase, &protectSize,
                                        PAGE_READWRITE, &stompOriginalProtection);
    if (status < 0) {
        printf("[IBM] Cannot open the image-code callback for writing: 0x%08lX\n",
               static_cast<unsigned long>(status));
        ntUnmapViewOfSection(hProcess, remoteView);
        return 0;
    }
    written = 0;
    status = SysNtWriteVirtualMemory(hProcess, reinterpret_cast<void*>(stompAddress),
                                      bootstrap.data(), bootstrap.size(), &written);
    void* restoreProtectBase = reinterpret_cast<void*>(stompAddress);
    SIZE_T restoreProtectSize = bootstrap.size();
    ULONG ignoredProtection = 0;
    SysNtProtectVirtualMemory(hProcess, &restoreProtectBase, &restoreProtectSize,
                              PAGE_EXECUTE_READ, &ignoredProtection);
    FlushInstructionCache(hProcess, reinterpret_cast<void*>(stompAddress), bootstrap.size());
    if (status < 0 || written != bootstrap.size()) {
        printf("[IBM] Could not write the image-code callback: 0x%08lX\n",
               static_cast<unsigned long>(status));
        protectBase = reinterpret_cast<void*>(stompAddress);
        protectSize = bootstrap.size();
        SysNtProtectVirtualMemory(hProcess, &protectBase, &protectSize,
                                  PAGE_READWRITE, &ignoredProtection);
        SysNtWriteVirtualMemory(hProcess, reinterpret_cast<void*>(stompAddress),
                                originalCode.data(), originalCode.size(), &written);
        protectBase = reinterpret_cast<void*>(stompAddress);
        protectSize = bootstrap.size();
        SysNtProtectVirtualMemory(hProcess, &protectBase, &protectSize,
                                  stompOriginalProtection, &ignoredProtection);
        FlushInstructionCache(hProcess, reinterpret_cast<void*>(stompAddress), bootstrap.size());
        ntUnmapViewOfSection(hProcess, remoteView);
        return 0;
    }

    TpTaskCallbacks728 callbacks = { reinterpret_cast<void*>(stompAddress), nullptr };
    TpDirect728 direct = {};
    direct.task.callbacks = reinterpret_cast<TpTaskCallbacks728*>(callbacksAddress);
    direct.callback = reinterpret_cast<void*>(stompAddress);
    status = SysNtWriteVirtualMemory(hProcess, reinterpret_cast<void*>(callbacksAddress),
                                      &callbacks, sizeof(callbacks), &written);
    if (status >= 0)
        status = SysNtWriteVirtualMemory(hProcess, reinterpret_cast<void*>(directAddress),
                                          &direct, sizeof(direct), &written);

    bool entryCompleted = false;
    if (status >= 0) {
        auto completionHandles = FindIoCompletionHandles728(hProcess, GetProcessId(hProcess));
        printf("[IBM] Found %zu IoCompletion handle(s)\n", completionHandles.size());
        for (HANDLE completion : completionHandles) {
            status = SysNtSetIoCompletion(completion, directAddress, nullptr, 0, 0);
            printf("[IBM] NtSetIoCompletion: 0x%08lX\n",
                   static_cast<unsigned long>(status));
            if (status < 0)
                continue;
            for (int wait = 0; wait < 600; ++wait) {
                LONG flag = 0;
                SIZE_T flagBytes = 0;
                SysNtReadVirtualMemory(hProcess, reinterpret_cast<void*>(flagAddress),
                                        &flag, sizeof(flag), &flagBytes);
                if (flag == 1) {
                    entryCompleted = true;
                    break;
                }
                if (WaitForSingleObject(hProcess, 0) == WAIT_OBJECT_0)
                    break;
                Sleep(50);
            }
            if (entryCompleted || status >= 0)
                break;
        }
        for (HANDLE completion : completionHandles)
            CloseHandle(completion);
    }

    if (entryCompleted)
        Sleep(10);
    protectBase = reinterpret_cast<void*>(stompAddress);
    protectSize = bootstrap.size();
    status = SysNtProtectVirtualMemory(hProcess, &protectBase, &protectSize,
                                        PAGE_READWRITE, &ignoredProtection);
    if (status >= 0) {
        SysNtWriteVirtualMemory(hProcess, reinterpret_cast<void*>(stompAddress),
                                originalCode.data(), originalCode.size(), &written);
        protectBase = reinterpret_cast<void*>(stompAddress);
        protectSize = bootstrap.size();
        SysNtProtectVirtualMemory(hProcess, &protectBase, &protectSize,
                                  stompOriginalProtection, &ignoredProtection);
        FlushInstructionCache(hProcess, reinterpret_cast<void*>(stompAddress), bootstrap.size());
        printf("[IBM] Restored the %ls image-code callback\n", stompName.c_str());
    }

    if (!entryCompleted) {
        printf("[IBM] Payload entry point did not complete\n");
        ntUnmapViewOfSection(hProcess, remoteView);
        return 0;
    }
    printf("[IBM] Image-backed manual map completed at 0x%llX\n",
           static_cast<unsigned long long>(remoteBase));
    return remoteBase;
}
