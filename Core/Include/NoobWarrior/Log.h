// === noobWarrior ===
// File: Log.h
// Started by: Hattozo
// Started on: 3/6/2025
// Description:
#pragma once
#include <string_view>
#include <iostream>
#include <format>

namespace NoobWarrior {
extern bool gLog_PrintToStdOut;
enum class Level {
    Info,
    Warn,
    Error,
    Fatal
};

template <typename... Args>
void OutEx(std::ostream *stream, std::string_view category, std::format_string<Args...> fmt, Args...args) {
    if (stream == &std::cout && !gLog_PrintToStdOut)
        return;
    std::string fmtStr = std::format("[NoobWarrior::{}] ", category) + std::format(fmt, std::forward<Args>(args)...);
    *stream << fmtStr << std::endl;
    if (stream != &std::cout && gLog_PrintToStdOut)
        std::cout << fmtStr << std::endl;
}

template <typename... Args>
void Out(std::string_view category, std::format_string<Args...> fmt, Args...args) {
    OutEx(&std::cout, category, fmt, std::forward<Args>(args)...);
}
}