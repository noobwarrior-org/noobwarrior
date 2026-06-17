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
 // File: Instance.h
 // Started by: Hattozo
 // Started on: 6/15/2026
 // Description:
#pragma once
#include "ScriptExecutor.h"

#include <cstdint>
#include <vector>
#include <memory>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../ntdll.h"

#include <Luau/Compiler.h>
#include <Luau/BytecodeBuilder.h>

namespace NoobHook::ScriptExecutor {
class BytecodeEncoderClient : public Luau::BytecodeEncoder {
    uint8_t encodeOp(uint8_t op) override {
        return uint8_t(op * 227);
    }
};

class Instance {
public:
    Instance(uintptr_t address);

    std::vector<uintptr_t> GetChildrenAddresses() const;
    std::vector<std::unique_ptr<Instance>> GetChildren() const;
    std::uintptr_t FindFirstChildAddress(const std::string_view name) const;
    std::unique_ptr<Instance> FindFirstChild(const std::string_view name) const;
    std::uintptr_t WaitForChildAddress(const std::string_view name, int timeout = 9e9) const;
    std::unique_ptr<Instance> WaitForChild(const std::string_view name, int timeout = 9e9) const;
    std::uintptr_t FindFirstChildOfClassAddress(const std::string_view className) const;
    std::unique_ptr<Instance> FindFirstChildOfClass(const std::string_view className) const;

    bool SetBytecode(const std::string& compressedBytecode, bool revertBytecode = false) const;
    std::string GetBytecode() const;

    void UnlockModule() const;

    std::vector<std::string> GetProperties() const;

    inline std::uintptr_t GetAddress() const {
        return mAddress;
    }

    inline std::string GetName() const {
        return ReadRobloxString(NoobHook::ReadPrimitive<uintptr_t>(mAddress + Offsets::Name));
    }

    inline std::string ClassName() const {
        return ReadRobloxString(NoobHook::ReadPrimitive<uintptr_t>(mAddress + Offsets::ClassName));
    }

    inline uintptr_t GetParentAddress() const {
        return NoobHook::ReadPrimitive<uintptr_t>(mAddress + Offsets::Parent);
    }

    inline std::unique_ptr<Instance> GetParent() const {
        return std::make_unique<Instance>(GetParentAddress());
    }
private:
    const uintptr_t mAddress;
};
}