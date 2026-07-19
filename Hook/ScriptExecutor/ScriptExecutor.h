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

void Install();
uintptr_t FindDataModelAddress();
void Execute(const std::string& source);
}