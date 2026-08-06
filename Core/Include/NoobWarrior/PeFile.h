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
// File: PeFile.h
// Started by: Hattozo
// Started on: 8/5/2026
// Description: Reads metadata out of Windows PE executables without any Win32 API, so a binary can
//              be inspected identically on every host OS.
#pragma once
#include <filesystem>
#include <string>

namespace NoobWarrior::Pe {
enum class Machine {
    Unknown,
    x86,
    x86_64
};

// The COFF machine type. Returns Unknown if the file is missing, unreadable or not a PE at all.
Machine ReadMachine(const std::filesystem::path &path);

// The ProductVersion string from the PE's version resource, e.g. "0.463.0.417004".
// Empty if the file has no usable version resource.
std::string ReadProductVersion(const std::filesystem::path &path);
}
