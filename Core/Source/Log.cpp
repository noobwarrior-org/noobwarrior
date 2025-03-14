// === noobWarrior ===
// File: Log.cpp
// Started by: Hattozo
// Started on: 3/6/2025
// Description:
#include <NoobWarrior/Log.h>

using namespace NoobWarrior;

bool NoobWarrior::gLog_PrintToStdOut = true;

static const char* MapLevelToString(Level lv) {
    switch (lv) {
    case Level::Info:
        return "Info";
    case Level::Warn:
        return "Warn";
    case Level::Error:
        return "Error";
    case Level::Fatal:
        return "Fatal";
    }
}