// === noobWarrior ===
// File: Log.h
// Started by: Hattozo
// Started on: 3/6/2025
// Description:
#pragma once
#define NOOBWARRIOR_OUT(category, fmt, ...) NoobWarrior::Out(stdout, category, fmt, ##__VA_ARGS__);
#include <string_view>

namespace NoobWarrior {
extern bool gLog_PrintToStdOut;
enum class Level {
    Info,
    Warn,
    Error,
    Fatal
};
void Out(FILE* stream, const char* category, const char* fmt, ...);
}