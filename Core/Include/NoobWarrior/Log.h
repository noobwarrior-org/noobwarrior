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
// File: Log.h
// Started by: Hattozo
// Started on: 3/6/2025
// Description:
#pragma once
#include <string_view>
#include <iostream>
#include <format>
#include <mutex>
#include <filesystem>
#include <fstream>

#include <NoobWarrior/Date.h>

namespace NoobWarrior {
extern std::mutex gLog_Mutex;
extern bool gLog_PrintToStdOut;

inline std::filesystem::path (*gLog_ResolveLogPath)() { nullptr };

enum class Level {
    Info,
    Warn,
    Error,
    Fatal
};

template <typename... Args>
void OutTo(std::ostream *stream, const std::filesystem::path &logPath, std::string_view category, std::string_view fmt, Args...args) {
    std::lock_guard<std::mutex> lock(gLog_Mutex);
    std::string fmtStr = Date::GetCurrentTimeAsString() + " " + std::vformat("[NoobWarrior::{}] ", std::make_format_args(category)) + std::vformat(fmt, std::make_format_args(args...));
    if (stream != &std::cout || gLog_PrintToStdOut) {
        *stream << fmtStr << std::endl;
        if (stream != &std::cout && gLog_PrintToStdOut)
            std::cout << fmtStr << std::endl;
    }
    if (logPath.empty())
        return;
    std::ofstream fileStream(logPath, std::ios_base::app);
    if (fileStream.is_open())
        fileStream << fmtStr << "\n";
}

template <typename... Args>
void OutEx(std::ostream *stream, std::string_view category, std::string_view fmt, Args...args) {
    OutTo(stream, gLog_ResolveLogPath != nullptr ? gLog_ResolveLogPath() : std::filesystem::path {},
          category, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Out(std::string_view category, std::string_view fmt, Args...args) {
    OutEx(&std::cout, category, fmt, std::forward<Args>(args)...);
}
}