// === noobWarrior ===
// File: Macros.h
// Started by: Hattozo
// Started on: 6/18/2025
// Description: Contains useful macros
#pragma once

#define NOOBWARRIOR_VERSION "0.0.2"
#define NOOBWARRIOR_AUTHORS \
"Hattozo - Creator of the noobWarrior project and software\n"
#define NOOBWARRIOR_CONTRIBUTORS \
"Hattozo\n"
#define NOOBWARRIOR_ATTRIBUTIONS_BRIEF \
"noobWarrior (https://github.com/noobWarrior-org/noobWarrior), licensed under the MIT License (https://opensource.org/license/mit/)\n" \
"curl (https://github.com/curl/curl), licensed under the curl license (https://curl.se/docs/copyright.html)\n" \
"SQLite (https://github.com/sqlite/sqlite/tree/master) - This is under the public domain.\n" \
"yaml-cpp (https://github.com/jbeder/yaml-cpp), licensed under the MIT License (https://opensource.org/license/mit/)\n" \
"nlohmann/json (https://github.com/nlohmann/json), licensed under the MIT License (https://opensource.org/license/mit/)\n" \
"Roblox-File-Format (https://github.com/MaximumADHD/Roblox-File-Format), licensed under the MIT License (https://opensource.org/license/mit/)\n" \
"famfamfam (Mark James) silk icons (https://github.com/markjames/famfamfam-silk-icons), licensed under the Creative Commons Attribution 2.5 License (http://creativecommons.org/licenses/by/2.5/)\n"

#define NOOBWARRIOR_FREE_PTR(ptr) delete ptr; ptr = nullptr;
#define NOOBWARRIOR_ARRAY_SIZE(arr) sizeof(arr) / sizeof(arr[0])