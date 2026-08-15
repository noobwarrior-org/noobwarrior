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
// File: ManualMap.h
// Started by: Hattozo
// Started on: 8/12/2025
// Description: Manual mapper - loads a DLL without going through the Windows loader,
// bypassing Hyperion's loader hooks.
// Slopped by DeepSeek V4 Pro and GPT 5.6 Sol.
#pragma once
#include <windows.h>
#include <cstdint>
#include <vector>
#include <string>

// Read DLL file into memory.
std::vector<uint8_t> ReadDllFile(const wchar_t* path);

// Manually map a DLL into the target process.
// Allocates memory, copies sections, fixes relocations, resolves imports,
// calls TLS callbacks and DllMain. Returns the remote image base or 0.
uintptr_t ManualMapDll(HANDLE hProcess, const std::vector<uint8_t>& dllData);

// Map a DLL over a second SEC_IMAGE view of a loaded system DLL.  This keeps
// the payload executable pages image-backed and avoids modifying Hyperion's
// own .byfron view.  Used by the 0.728 Player path.
uintptr_t ImageBackedManualMapDll(HANDLE hProcess, const std::vector<uint8_t>& dllData,
                                  bool runEntryPoint = true,
                                  const wchar_t* earlyBackingPath = nullptr);

// Return an exported symbol's image-relative address from raw PE data.
uintptr_t GetImageExportRva(const std::vector<uint8_t>& dllData, const char* exportName);
