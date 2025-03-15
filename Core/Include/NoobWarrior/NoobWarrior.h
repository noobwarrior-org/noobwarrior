// === noobWarrior ===
// File: NoobWarrior.h
// Started by: Hattozo
// Started on: 12/15/2024
// Description: Contains global constants, commonly used headers, and useful macros
#pragma once

// Utility macros
#define NOOBWARRIOR_VERSION "0.0.1"
#define NOOBWARRIOR_AUTHORS \
"Hattozo - Creator of the noobWarrior project and software\n"
#define NOOBWARRIOR_CONTRIBUTORS \
"Hattozo\n"
#define NOOBWARRIOR_ATTRIBUTIONS_BRIEF \
"noobWarrior (https://github.com/noobWarrior-org/noobWarrior), licensed under the MIT license (https://opensource.org/license/mit/)\n" \
"Roblox-File-Format (https://github.com/MaximumADHD/Roblox-File-Format), licensed under the MIT license (https://opensource.org/license/mit/)\n" \
"yaml-cpp (https://github.com/jbeder/yaml-cpp), licensed under the MIT license (https://opensource.org/license/mit/)\n"

#define NOOBWARRIOR_FREE_PTR(ptr) delete ptr; ptr = nullptr;
#define NOOBWARRIOR_ARRAY_SIZE(arr) sizeof(arr) / sizeof(arr[0])

// Include commonly used headers
#include "Log.h"
#include "Archive.h"
#include "Auth.h"
#include "Config.h"
#include "RccServiceInvoker.h"