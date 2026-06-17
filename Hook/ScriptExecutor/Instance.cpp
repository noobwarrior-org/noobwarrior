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
 // File: Instance.cpp
 // Started by: Hattozo
 // Started on: 6/15/2026
 // Description:
#include "Instance.h"
#include "ScriptExecutor.h"

#define XXH_INLINE_ALL
#include <xxhash.h>
#include <zstd.h>

using namespace NoobHook::ScriptExecutor;

static std::string compress(const std::string_view bytecode) {
    const auto data_size = bytecode.size();
    const auto max_size = ZSTD_compressBound(data_size);
    auto buffer = std::vector<char>(max_size + 8);

    strcpy_s(&buffer[0], buffer.capacity(), "RSB1");
    memcpy_s(&buffer[4], buffer.capacity(), &data_size, sizeof(data_size));

    const auto compressed_size = ZSTD_compress(&buffer[8], max_size, bytecode.data(), data_size, ZSTD_maxCLevel());
    if (ZSTD_isError(compressed_size))
        return "";

    const auto size = compressed_size + 8;
    const auto key = XXH32(buffer.data(), size, 42u);
    const auto bytes = reinterpret_cast<const uint8_t*>(&key);

    for (auto i = 0u; i < size; ++i)
        buffer[i] ^= bytes[i % 4] + i * 41u;

    return std::string(buffer.data(), size);
}

static std::string decompress(const std::string_view compressed) {
    const uint8_t bytecodeSignature[4] = { 'R', 'S', 'B', '1' };
    const int bytecodeHashMultiplier = 41;
    const int bytecodeHashSeed = 42;

    if (compressed.size() < 8)
        return "Compressed data too short";

    std::vector<uint8_t> compressedData(compressed.begin(), compressed.end());
    std::vector<uint8_t> headerBuffer(4);

    for (size_t i = 0; i < 4; ++i) {
        headerBuffer[i] = compressedData[i] ^ bytecodeSignature[i];
        headerBuffer[i] = (headerBuffer[i] - i * bytecodeHashMultiplier) % 256;
    }

    for (size_t i = 0; i < compressedData.size(); ++i) {
        compressedData[i] ^= (headerBuffer[i % 4] + i * bytecodeHashMultiplier) % 256;
    }

    uint32_t hashValue = 0;
    for (size_t i = 0; i < 4; ++i) {
        hashValue |= headerBuffer[i] << (i * 8);
    }

    uint32_t rehash = XXH32(compressedData.data(), compressedData.size(), bytecodeHashSeed);
    if (rehash != hashValue)
        return "Hash mismatch during decompression";

    uint32_t decompressedSize = 0;
    for (size_t i = 4; i < 8; ++i) {
        decompressedSize |= compressedData[i] << ((i - 4) * 8);
    }

    compressedData = std::vector<uint8_t>(compressedData.begin() + 8, compressedData.end());
    std::vector<uint8_t> decompressed(decompressedSize);

    size_t const actualDecompressedSize = ZSTD_decompress(decompressed.data(), decompressedSize, compressedData.data(), compressedData.size());
    if (ZSTD_isError(actualDecompressedSize))
        return "ZSTD decompression error: " + std::string(ZSTD_getErrorName(actualDecompressedSize));

    decompressed.resize(actualDecompressedSize);
    return std::string(decompressed.begin(), decompressed.end());
}

static std::string compilable(const std::string& source, bool returnBytecode) {
    static NoobHook::ScriptExecutor::BytecodeEncoderClient encoder = NoobHook::ScriptExecutor::BytecodeEncoderClient();
    std::string bytecode = Luau::compile(source, {}, {}, &encoder);
    if (bytecode[0] == '\0') {
        bytecode.erase(std::remove(bytecode.begin(), bytecode.end(), '\0'), bytecode.end());
        return bytecode;
    }
    if (returnBytecode) {
        return bytecode;
    }
    return "success";
}

static std::string Compile(const std::string& source) {
    static NoobHook::ScriptExecutor::BytecodeEncoderClient encoder = NoobHook::ScriptExecutor::BytecodeEncoderClient();
    const std::string bytecode = Luau::compile(source, {}, {}, &encoder);

    if (bytecode[0] == '\0') {
        std::string bytecodeP = bytecode;
        bytecodeP.erase(std::remove(bytecodeP.begin(), bytecodeP.end(), '\0'), bytecodeP.end());
        NoobHook::Out("ScriptExecutor::Compile", "Bytecode compile failed: %s", bytecodeP.c_str());
    }

    return compress(bytecode);
}

Instance::Instance(uintptr_t address) :
    mAddress(address)
{
}

std::vector<uintptr_t> Instance::GetChildrenAddresses() const {
    return NoobHook::ScriptExecutor::GetChildrenAddresses(mAddress);
}

std::vector<std::unique_ptr<Instance>> Instance::GetChildren() const {
    std::vector<std::uintptr_t> childAddresses = GetChildrenAddresses();
    std::vector<std::unique_ptr<Instance>> children;
    for (std::uintptr_t address : childAddresses) {
        children.push_back(std::make_unique<Instance>(address));
    }
    return children;
}

std::uintptr_t Instance::FindFirstChildAddress(const std::string_view name) const {
    std::vector<std::uintptr_t> childAddresses = GetChildrenAddresses();
    for (std::uintptr_t address : childAddresses) {
        if (ReadRobloxString(NoobHook::ReadPrimitive<std::uintptr_t>(address + Offsets::Name)) == name)
            return address;
    }
    return 0;
}

std::unique_ptr<Instance> Instance::FindFirstChild(const std::string_view name) const {
    std::uintptr_t childAddress = FindFirstChildAddress(name);
    if (childAddress != 0)
        return std::make_unique<Instance>(childAddress);
    return nullptr;
}

std::uintptr_t Instance::WaitForChildAddress(const std::string_view name, int timeout) const {
    std::uintptr_t existing = FindFirstChildAddress(name);
    if (existing != 0)
        return existing;

    std::chrono::steady_clock::time_point start_time = std::chrono::high_resolution_clock::now();
    auto timeout_duration = std::chrono::seconds(timeout);

    while (std::chrono::high_resolution_clock::now() - start_time <= timeout_duration) {
        if (FindFirstChildAddress(name))
            return FindFirstChildAddress(name);
        Sleep(100);
    }
    return 0;
}

std::unique_ptr<Instance> Instance::WaitForChild(const std::string_view name, int timeout) const {
	std::uintptr_t childAddress = WaitForChildAddress(name, timeout);
	if (childAddress != 0)
		return std::make_unique<Instance>(childAddress);
	return nullptr;
}

std::uintptr_t Instance::FindFirstChildOfClassAddress(const std::string_view className) const {
    std::vector<std::uintptr_t> childAddresses = GetChildrenAddresses();
    for (std::uintptr_t address : childAddresses) {
        if (ReadRobloxString(NoobHook::ReadPrimitive<uintptr_t>(NoobHook::ReadPrimitive<uintptr_t>(address + Offsets::ClassDescriptor) + Offsets::ClassName)) == className)
            return address;
    }
    return 0;
}

std::unique_ptr<Instance> Instance::FindFirstChildOfClass(const std::string_view className) const {
    std::uintptr_t childAddress = FindFirstChildOfClassAddress(className);
    if (childAddress != 0)
        return std::make_unique<Instance>(childAddress);
    return nullptr;
}

bool Instance::SetBytecode(const std::string& compressedBytecode, bool revertBytecode) const {
    if (ClassName() != "LocalScript" && ClassName() != "ModuleScript")
        return false;

    uintptr_t embeddedSourceOffset = (ClassName() == "LocalScript") ? Offsets::LocalScriptEmbedded : Offsets::ModuleScriptEmbedded;
    uintptr_t embeddedPtr = NoobHook::ReadPrimitive<uintptr_t>(mAddress + embeddedSourceOffset);

    if (revertBytecode) {
        uintptr_t originalBytecodePtr = NoobHook::ReadPrimitive<uintptr_t>(embeddedPtr + Offsets::Bytecode);
        uint64_t originalSize = NoobHook::ReadPrimitive<uint64_t>(embeddedPtr + Offsets::BytecodeSize);

        std::thread([embeddedPtr, originalBytecodePtr, originalSize]() {
            Sleep(850);
            NoobHook::WritePrimitive<uintptr_t>(embeddedPtr + Offsets::Bytecode, originalBytecodePtr);
            NoobHook::WritePrimitive<uint64_t>(embeddedPtr + Offsets::BytecodeSize, originalSize);
        }).detach();
    }

    LPVOID allocatedAddress = VirtualAllocEx(GetCurrentProcess(), nullptr, compressedBytecode.size(), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (allocatedAddress == 0)
        return false;

    DWORD oldProtection;
    DWORD d;

    VirtualProtectEx(GetCurrentProcess(), allocatedAddress, sizeof(compressedBytecode.c_str()), PAGE_READWRITE, &oldProtection);
    NtWriteVirtualMemory(GetCurrentProcess(), allocatedAddress, (PVOID)compressedBytecode.c_str(), (ULONG)compressedBytecode.size(), nullptr);

    return VirtualProtectEx(GetCurrentProcess(), allocatedAddress, sizeof(compressedBytecode.c_str()), oldProtection, &d)
        && NoobHook::WritePrimitive<uintptr_t>(embeddedPtr + Offsets::Bytecode, reinterpret_cast<uintptr_t>(allocatedAddress))
        && NoobHook::WritePrimitive<uint64_t>(embeddedPtr + Offsets::BytecodeSize, compressedBytecode.size());
}

std::string Instance::GetBytecode() const {
    if (ClassName() != "LocalScript" && ClassName() != "ModuleScript")
        return "";

    std::uintptr_t embeddedSourceOffset = (ClassName() == "LocalScript") ? Offsets::LocalScriptEmbedded : Offsets::ModuleScriptEmbedded;
    std::uintptr_t embeddedPtr = NoobHook::ReadPrimitive<std::uintptr_t>(mAddress + embeddedSourceOffset);

    std::uintptr_t bytecodePtr = NoobHook::ReadPrimitive<std::uintptr_t>(embeddedPtr + Offsets::Bytecode);
    std::uint64_t bytecodeSize = NoobHook::ReadPrimitive<std::uint64_t>(embeddedPtr + Offsets::BytecodeSize);

    std::string bytecodeBuffer;
    bytecodeBuffer.resize(bytecodeSize);

    MEMORY_BASIC_INFORMATION bi;
    VirtualQueryEx(GetCurrentProcess(), reinterpret_cast<LPCVOID>(bytecodePtr), &bi, sizeof(bi));

    NtReadVirtualMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(bytecodePtr), bytecodeBuffer.data(), (ULONG)bytecodeSize, nullptr);

    PVOID baddr = bi.AllocationBase;
    SIZE_T size = bi.RegionSize;
    NtUnlockVirtualMemory(GetCurrentProcess(), &baddr, &size, 1);

    return decompress(bytecodeBuffer);
}

void Instance::UnlockModule() const {
    if (ClassName() == "ModuleScript") {
        NoobHook::WritePrimitive<unsigned long long>(GetAddress() + Offsets::ModuleFlags, 0x100000000);
        NoobHook::WritePrimitive<uintptr_t>(GetAddress() + Offsets::IsCoreScript, 0x1);
    }
}

std::vector<std::string> Instance::GetProperties() const {
    std::vector<std::string> Properties;
    {
        uintptr_t ClassDescriptor = NoobHook::ReadPrimitive<uintptr_t>(mAddress + Offsets::ClassDescriptor);

        uintptr_t PropertiesStart = NoobHook::ReadPrimitive<uintptr_t>(ClassDescriptor + 0x28);
        uintptr_t PropertiesEnd = NoobHook::ReadPrimitive<uintptr_t>(ClassDescriptor + 0x30);

        for (uintptr_t PropertyAddress = PropertiesStart; PropertyAddress < PropertiesEnd; PropertyAddress += 0x8) {
            uintptr_t PropertyPtr = NoobHook::ReadPrimitive<uintptr_t>(PropertyAddress);
            if (PropertyPtr != 0)
                Properties.push_back(ReadRobloxString(NoobHook::ReadPrimitive<uintptr_t>(PropertyPtr + 0x8)));
        }
    }
    return Properties;
}