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

void NoobWarrior::Out(FILE *stream, const char* category, const char *fmt, ...) {
    if (stream == stdout && !gLog_PrintToStdOut)
        return;
    int size = strlen(fmt) + strlen(category) + 18;
    char* newFmt = (char*)malloc(size);
    snprintf(newFmt, size, "[NoobWarrior::%s] %s\n", category, fmt);
    va_list list;
    va_start(list, newFmt);
    vprintf(newFmt, list);
    if (stream != stdout && gLog_PrintToStdOut)
        vfprintf(stream, newFmt, list);
    va_end(list);
}