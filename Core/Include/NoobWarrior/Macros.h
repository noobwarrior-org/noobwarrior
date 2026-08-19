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
// File: Macros.h
// Started by: Hattozo
// Started on: 6/18/2025
// Description: Contains useful macros
#pragma once

#define NOOBWARRIOR_BRAND "noobWarrior"
#define NOOBWARRIOR_VERSION "0.0.8"
#define NOOBWARRIOR_AUTHORS \
"Hattozo - Creator of the " NOOBWARRIOR_BRAND " project and software\n"
#define NOOBWARRIOR_MAINTAINERS \
"Hattozo\n"
#define NOOBWARRIOR_CONTRIBUTORS \
"VisualPlugin\n" \
"Heraklis\n"
#define NOOBWARRIOR_ATTRIBUTIONS_BRIEF \
"noobWarrior (https://github.com/noobWarrior-org/noobWarrior), licensed under the LGPLv3 License (https://www.gnu.org/licenses/lgpl-3.0.html)\n" \
"curl (https://github.com/curl/curl), licensed under the curl license (https://curl.se/docs/copyright.html)\n" \
"SQLite (https://github.com/sqlite/sqlite/tree/master) - This is under the public domain.\n" \
"nlohmann/json (https://github.com/nlohmann/json), licensed under the MIT License (https://opensource.org/license/mit/)\n" \
"LuaJIT (https://github.com/luajit/luajit), licensed under the MIT License (https://opensource.org/license/mit/)\n" \
"sol2 (https://github.com/ThePhD/sol2), licensed under the MIT License (https://opensource.org/license/mit/)\n" \
"libevent (https://github.com/libevent/libevent), licensed under the 3-clause BSD license (https://github.com/libevent/libevent/blob/master/LICENSE)\n" \
"libzip (https://github.com/nih-at/libzip), licensed under the 3-clause BSD license (https://github.com/libevent/libevent/blob/master/LICENSE)\n" \
"zlib (https://github.com/madler/zlib), licensed under the zlib license (https://github.com/madler/zlib/blob/develop/LICENSE)\n" \
"zstd (https://github.com/facebook/zstd), licensed under the 3-clause BSD License (https://github.com/facebook/zstd/blob/dev/LICENSE)\n" \
"pugixml (https://github.com/zeux/pugixml), licensed under the MIT License (https://opensource.org/license/mit/)\n" \
"Roblox-File-Format (https://github.com/MaximumADHD/Roblox-File-Format), licensed under the MIT License (https://opensource.org/license/mit/)\n" \
"libcppgenerate (https://github.com/rm5248/libcppgenerate), licensed under the Apache License 2.0 (https://www.apache.org/licenses/LICENSE-2.0)\n" \
"famfamfam (Mark James) silk icons (https://github.com/markjames/famfamfam-silk-icons), licensed under the Creative Commons Attribution 2.5 License (http://creativecommons.org/licenses/by/2.5/)\n" \
"local_rcc (https://github.com/rsblox/local_rcc), licensed under the MIT License (https://opensource.org/license/mit/)\n" \
"studio-offline (https://github.com/Roblox-Devs/studio-offline), licensed under the WTFPL license (https://github.com/Roblox-Devs/studio-offline/blob/main/LICENSE)\n" \
"MinHook (https://github.com/tsudakageyu/minhook), licensed under the 2-clause BSD license (https://github.com/TsudaKageyu/minhook/blob/master/LICENSE.txt)\n" \
"BLAKE3 (https://github.com/BLAKE3-team/BLAKE3), licensed under the CC0 1.0 (https://github.com/BLAKE3-team/BLAKE3/blob/master/LICENSE_CC0)\n"

#define NOOBWARRIOR_FREE_PTR(ptr) delete ptr; ptr = nullptr;
#define NOOBWARRIOR_ARRAY_SIZE(arr) sizeof(arr) / sizeof(arr[0])
