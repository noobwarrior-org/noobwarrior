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