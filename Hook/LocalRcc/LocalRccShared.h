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
// File: LocalRccShared.h
// Description: Small set of LocalRCC facilities shared by versioned modules.

#pragma once

#include <cstdint>
#include <initializer_list>

namespace LocalRccShared {
void Log(const char* category, const char* format, ...);
void* ScanAny(const char* label, std::initializer_list<const char*> patterns);
std::uint8_t* GetExactStudio719ImageBase();
}
