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
// File: ScriptExecutor.h
// Started by: Hattozo
// Started on: 6/14/2026
// Description: A working script executor!
// Skidded from Xeno because I'm not intelligent
// You may ask, why is there a script executor here? Because of the plugins system in noobWarrior, which allows developers to insert models into the DataModel.
// That sounds like a terrible idea, but trust me, I'm all for a moddable experience.
// Besides, if you're that concerned, write your god damn code to never trust the client. Geez.
#pragma once
#include "../Hook.h"

#include <string>
#include <cstdint>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>

namespace NoobHook::ScriptExecutor {
extern uintptr_t gDataModelAddress;

namespace Offsets {
    // Instance
    constexpr std::uint64_t This = 0x8;
    constexpr std::uint64_t Name = 0x50; // 0x48
    constexpr std::uint64_t Children = 0x58; // 0x50
    constexpr std::uint64_t Parent = 0x28; // 0x60

    constexpr std::uint64_t ClassDescriptor = 0x18;
    constexpr std::uint64_t ClassName = 0x8;

    // Scripts
    constexpr std::uint64_t ModuleScriptEmbedded = 0x160;
    constexpr std::uint64_t IsCoreScript = 0x1a8;
    constexpr std::uint64_t ModuleFlags = IsCoreScript - 0x4;
    constexpr std::uint64_t LocalScriptEmbedded = 0x1c0;

    constexpr std::uint64_t Bytecode = 0x10;
    constexpr std::uint64_t BytecodeSize = 0x20;

    // Other
    constexpr std::uint64_t LocalPlayer = 0x110; // 0x100
    constexpr std::uint64_t ObjectValue = 0xc0;
}

std::vector<uintptr_t> GetChildrenAddresses(uintptr_t address);
std::string ReadRobloxString(uintptr_t address);

void Install();
uintptr_t FindDataModelAddress();
void Execute(const std::string& source);
}