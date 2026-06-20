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

#define NOOBWARRIOR_VERSION "0.0.6"
#define NOOBWARRIOR_AUTHORS \
"Hattozo - Creator of the noobWarrior project and software\n"
#define NOOBWARRIOR_CONTRIBUTORS \
"Hattozo\n"
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
"Xeno (https://github.com/lvplay2/Xeno), licensed under the Apache License 2.0 (https://github.com/lvplay2/Xeno/blob/main/LICENSE)\n" \
"famfamfam (Mark James) silk icons (https://github.com/markjames/famfamfam-silk-icons), licensed under the Creative Commons Attribution 2.5 License (http://creativecommons.org/licenses/by/2.5/)\n"

#define NOOBWARRIOR_FREE_PTR(ptr) delete ptr; ptr = nullptr;
#define NOOBWARRIOR_ARRAY_SIZE(arr) sizeof(arr) / sizeof(arr[0])