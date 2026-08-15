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
// File: Syscalls.cpp
// Started by: Hattozo
// Started on: 8/12/2025
// Description: Syscall stubs - bypass ntdll hooks by calling syscall from private memory.
// Slopped by GPT 5.6 Sol and DeepSeek V4 Pro
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winternl.h>
#include <cstdio>
#include <string>
#include <unordered_map>
#include "Syscalls.hpp"

static DWORD GetSyscallId(const char* name) {
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) return 0;
    uint8_t* stub = (uint8_t*)GetProcAddress(ntdll, name);
    if (!stub) return 0;
    for (int j = 0; j < 30; j++) {
        if (stub[j] == 0xB8) {
            DWORD id = *(DWORD*)(stub + j + 1);
            return id;
        }
    }
    return 0;
}

static void* MakeStub(DWORD id) {
    uint8_t code[] = {
        0x4C, 0x8B, 0xD1,
        0xB8, (uint8_t)id, (uint8_t)(id>>8), (uint8_t)(id>>16), (uint8_t)(id>>24),
        0x0F, 0x05,
        0xC3
    };
    void* mem = VirtualAlloc(nullptr, sizeof(code), MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!mem) return nullptr;
    memcpy(mem, code, sizeof(code));
    FlushInstructionCache(GetCurrentProcess(), mem, sizeof(code));
    return mem;
}

struct Stub { void* fn; DWORD id; };
static std::unordered_map<std::string, Stub> gStubs;

bool InitSyscalls() {
    static const char* names[] = {
        "NtOpenProcess","NtAllocateVirtualMemory","NtFreeVirtualMemory",
        "NtWriteVirtualMemory","NtReadVirtualMemory","NtProtectVirtualMemory",
        "NtOpenThread","NtSuspendThread","NtResumeThread",
        "NtGetContextThread","NtCreateThreadEx",
        "NtQuerySystemInformation","NtDuplicateObject","NtQueryObject",
    };
    for (auto* name : names) {
        DWORD id = GetSyscallId(name);
        if (!id) return false;
        void* stub = MakeStub(id);
        if (!stub) return false;
        gStubs[name] = { stub, id };
    }
    return true;
}

template<typename F> static F GetStub(const char* name) {
    auto it = gStubs.find(name);
    return it != gStubs.end() ? (F)it->second.fn : nullptr;
}

NTSTATUS SysNtOpenProcess(HANDLE* h, ACCESS_MASK a, void* oa, void* cid) {
    auto fn = GetStub<NTSTATUS(NTAPI*)(HANDLE*,ACCESS_MASK,void*,void*)>("NtOpenProcess");
    return fn ? fn(h,a,oa,cid) : (NTSTATUS)0xC0000002;
}
NTSTATUS SysNtAllocateVirtualMemory(HANDLE h, void** b, ULONG_PTR zb, SIZE_T* sz, ULONG at, ULONG pr) {
    auto fn = GetStub<NTSTATUS(NTAPI*)(HANDLE,void**,ULONG_PTR,SIZE_T*,ULONG,ULONG)>("NtAllocateVirtualMemory");
    return fn ? fn(h,b,zb,sz,at,pr) : (NTSTATUS)0xC0000002;
}
NTSTATUS SysNtFreeVirtualMemory(HANDLE h, void** b, SIZE_T* sz, ULONG ft) {
    auto fn = GetStub<NTSTATUS(NTAPI*)(HANDLE,void**,SIZE_T*,ULONG)>("NtFreeVirtualMemory");
    return fn ? fn(h,b,sz,ft) : (NTSTATUS)0xC0000002;
}
NTSTATUS SysNtWriteVirtualMemory(HANDLE h, void* b, const void* buf, SIZE_T sz, SIZE_T* out) {
    auto fn = GetStub<NTSTATUS(NTAPI*)(HANDLE,void*,const void*,SIZE_T,SIZE_T*)>("NtWriteVirtualMemory");
    return fn ? fn(h,b,buf,sz,out) : (NTSTATUS)0xC0000002;
}
NTSTATUS SysNtReadVirtualMemory(HANDLE h, void* b, void* buf, SIZE_T sz, SIZE_T* out) {
    auto fn = GetStub<NTSTATUS(NTAPI*)(HANDLE,void*,void*,SIZE_T,SIZE_T*)>("NtReadVirtualMemory");
    return fn ? fn(h,b,buf,sz,out) : (NTSTATUS)0xC0000002;
}
NTSTATUS SysNtProtectVirtualMemory(HANDLE h, void** b, SIZE_T* sz, ULONG np, ULONG* op) {
    auto fn = GetStub<NTSTATUS(NTAPI*)(HANDLE,void**,SIZE_T*,ULONG,ULONG*)>("NtProtectVirtualMemory");
    return fn ? fn(h,b,sz,np,op) : (NTSTATUS)0xC0000002;
}
NTSTATUS SysNtOpenThread(HANDLE* h, ACCESS_MASK a, void* oa, void* cid) {
    auto fn = GetStub<NTSTATUS(NTAPI*)(HANDLE*,ACCESS_MASK,void*,void*)>("NtOpenThread");
    return fn ? fn(h,a,oa,cid) : (NTSTATUS)0xC0000002;
}
NTSTATUS SysNtSuspendThread(HANDLE h, ULONG* prev) {
    auto fn = GetStub<NTSTATUS(NTAPI*)(HANDLE,ULONG*)>("NtSuspendThread");
    return fn ? fn(h,prev) : (NTSTATUS)0xC0000002;
}
NTSTATUS SysNtResumeThread(HANDLE h, ULONG* prev) {
    auto fn = GetStub<NTSTATUS(NTAPI*)(HANDLE,ULONG*)>("NtResumeThread");
    return fn ? fn(h,prev) : (NTSTATUS)0xC0000002;
}
NTSTATUS SysNtGetContextThread(HANDLE h, CONTEXT* ctx) {
    auto fn = GetStub<NTSTATUS(NTAPI*)(HANDLE,CONTEXT*)>("NtGetContextThread");
    return fn ? fn(h,ctx) : (NTSTATUS)0xC0000002;
}
NTSTATUS SysNtCreateThreadEx(HANDLE* h, ACCESS_MASK a, void* oa, HANDLE hp, void* s, void* arg,
                              ULONG fl, ULONG_PTR zb, SIZE_T ss, SIZE_T ms, void* al) {
    auto fn = GetStub<NTSTATUS(NTAPI*)(HANDLE*,ACCESS_MASK,void*,HANDLE,void*,void*,ULONG,ULONG_PTR,SIZE_T,SIZE_T,void*)>("NtCreateThreadEx");
    return fn ? fn(h,a,oa,hp,s,arg,fl,zb,ss,ms,al) : (NTSTATUS)0xC0000002;
}

NTSTATUS SysNtQuerySystemInformation(ULONG cls, void* info, ULONG len, ULONG* retLen) {
    auto fn = GetStub<NTSTATUS(NTAPI*)(ULONG,void*,ULONG,ULONG*)>("NtQuerySystemInformation");
    return fn ? fn(cls, info, len, retLen) : (NTSTATUS)0xC0000002;
}

NTSTATUS SysNtDuplicateObject(HANDLE srcProc, HANDLE srcHandle, HANDLE dstProc, HANDLE* dstHandle,
                               ACCESS_MASK access, ULONG attrs, ULONG opts) {
    auto fn = GetStub<NTSTATUS(NTAPI*)(HANDLE,HANDLE,HANDLE,HANDLE*,ACCESS_MASK,ULONG,ULONG)>("NtDuplicateObject");
    return fn ? fn(srcProc, srcHandle, dstProc, dstHandle, access, attrs, opts) : (NTSTATUS)0xC0000002;
}

NTSTATUS SysNtQueryObject(HANDLE h, ULONG cls, void* info, ULONG len, ULONG* retLen) {
    auto fn = GetStub<NTSTATUS(NTAPI*)(HANDLE,ULONG,void*,ULONG,ULONG*)>("NtQueryObject");
    return fn ? fn(h, cls, info, len, retLen) : (NTSTATUS)0xC0000002;
}

NTSTATUS SysNtSetIoCompletion(HANDLE h, ULONG_PTR key, void* apc, NTSTATUS ioStatus, ULONG_PTR info) {
    // NtSetIoCompletion is not exported from ntdll — it's internal. Use GetProcAddress on the real ntdll.
    using Fn = NTSTATUS(NTAPI*)(HANDLE,ULONG_PTR,void*,NTSTATUS,ULONG_PTR);
    static Fn fn = (Fn)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtSetIoCompletion");
    return fn ? fn(h, key, apc, ioStatus, info) : (NTSTATUS)0xC0000002;
}
