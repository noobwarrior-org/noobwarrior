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
// File: Log.cpp
// Started by: Hattozo
// Started on: 3/6/2025
// Description:
#include <NoobWarrior/Log.h>

#ifdef __ANDROID__
#include <android/log.h>
#endif

using namespace NoobWarrior;

std::mutex NoobWarrior::gLog_Mutex;
bool NoobWarrior::gLog_PrintToStdOut = true;

#ifdef __ANDROID__
namespace {
struct LogcatStreambuf : public std::streambuf {
    std::string mLine;
    int overflow(int c) override {
        if (c == '\n' || c == EOF) {
            __android_log_print(ANDROID_LOG_INFO, "noobwarrior", "%s", mLine.c_str());
            mLine.clear();
        } else {
            mLine += static_cast<char>(c);
        }
        return c;
    }
};
static LogcatStreambuf sLogcatBuf;
static struct LogcatInstall { LogcatInstall() { std::cout.rdbuf(&sLogcatBuf); } } sInstall;
} // namespace
#endif

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