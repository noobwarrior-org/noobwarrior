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
void OutEx(std::ostream *stream, std::string_view category, std::string_view fmt, Args...args) {
    if (stream == &std::cout && !gLog_PrintToStdOut)
        return;
    std::string fmtStr = std::vformat("[NoobWarrior::{}] ", std::make_format_args(category)) + std::vformat(fmt, std::make_format_args(args...));
    *stream << fmtStr << std::endl;
    if (stream != &std::cout && gLog_PrintToStdOut)
        std::cout << fmtStr << std::endl;
}

template <typename... Args>
void Out(std::string_view category, std::string_view fmt, Args...args) {
    OutEx(&std::cout, category, fmt, std::forward<Args>(args)...);
}
}