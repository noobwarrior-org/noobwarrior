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
// File: Syscalls.hpp
// Started by: Hattozo
// Started on: 8/12/2025
// Description: Direct syscall stubs - bypass Hyperion's ntdll hooks by executing `syscall`
// from private memory instead of calling through ntdll.dll.
// Slopped by DeepSeek V4 Pro and GPT 5.6 Sol
#pragma once
#include <windows.h>
#include <winternl.h>
#include <cstdint>

// Resolve all syscall IDs from ntdll.dll on disk. Must be called once.
bool InitSyscalls();

// --- Syscall wrappers (use these instead of Win32 APIs) ---

NTSTATUS SysNtOpenProcess(HANDLE* ProcessHandle, ACCESS_MASK DesiredAccess,
                          void* ObjectAttributes, void* ClientId);

NTSTATUS SysNtAllocateVirtualMemory(HANDLE ProcessHandle, void** BaseAddress,
                                    ULONG_PTR ZeroBits, SIZE_T* RegionSize,
                                    ULONG AllocationType, ULONG Protect);

NTSTATUS SysNtFreeVirtualMemory(HANDLE ProcessHandle, void** BaseAddress,
                                SIZE_T* RegionSize, ULONG FreeType);

NTSTATUS SysNtWriteVirtualMemory(HANDLE ProcessHandle, void* BaseAddress,
                                 const void* Buffer, SIZE_T NumberOfBytesToWrite,
                                 SIZE_T* NumberOfBytesWritten);

NTSTATUS SysNtReadVirtualMemory(HANDLE ProcessHandle, void* BaseAddress,
                                void* Buffer, SIZE_T NumberOfBytesToRead,
                                SIZE_T* NumberOfBytesRead);

NTSTATUS SysNtProtectVirtualMemory(HANDLE ProcessHandle, void** BaseAddress,
                                   SIZE_T* RegionSize, ULONG NewProtect,
                                   ULONG* OldProtect);

NTSTATUS SysNtOpenThread(HANDLE* ThreadHandle, ACCESS_MASK DesiredAccess,
                         void* ObjectAttributes, void* ClientId);

NTSTATUS SysNtSuspendThread(HANDLE ThreadHandle, ULONG* PreviousSuspendCount);

NTSTATUS SysNtResumeThread(HANDLE ThreadHandle, ULONG* PreviousSuspendCount);

NTSTATUS SysNtGetContextThread(HANDLE ThreadHandle, CONTEXT* Context);

NTSTATUS SysNtCreateThreadEx(HANDLE* ThreadHandle, ACCESS_MASK DesiredAccess,
                             void* ObjectAttributes, HANDLE ProcessHandle,
                             void* StartRoutine, void* Argument, ULONG CreateFlags,
                             ULONG_PTR ZeroBits, SIZE_T StackSize, SIZE_T MaximumStackSize,
                             void* AttributeList);

NTSTATUS SysNtQuerySystemInformation(ULONG SystemInformationClass,
                                     void* SystemInformation, ULONG SystemInformationLength,
                                     ULONG* ReturnLength);

NTSTATUS SysNtDuplicateObject(HANDLE SourceProcessHandle, HANDLE SourceHandle,
                               HANDLE TargetProcessHandle, HANDLE* TargetHandle,
                               ACCESS_MASK DesiredAccess, ULONG HandleAttributes, ULONG Options);

NTSTATUS SysNtQueryObject(HANDLE Handle, ULONG ObjectInformationClass,
                          void* ObjectInformation, ULONG ObjectInformationLength,
                          ULONG* ReturnLength);

NTSTATUS SysNtSetIoCompletion(HANDLE IoCompletionHandle, ULONG_PTR KeyContext,
                               void* ApcContext, NTSTATUS IoStatus, ULONG_PTR IoStatusInformation);

// Direct LdrLoadDll — bypasses kernel32 LoadLibraryW which may be hooked
NTSTATUS SysLdrLoadDll(PWCHAR PathToFile, ULONG Flags, PUNICODE_STRING ModuleFileName, HANDLE* ModuleHandle);
