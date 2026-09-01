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
// File: Date.cpp
// Started by: Hattozo
// Started on: 8/31/2026
// Description: Makes ISO-8601 dates
#include <NoobWarrior/Date.h>

#include <chrono>
#include <iomanip>
#include <algorithm>
#include <sstream>

using namespace NoobWarrior;

std::string Date::GetCurrentTimeAsString(bool hasColons) {
    std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::stringstream stream;
    stream << std::put_time(std::localtime(&t), "%FT%T%z");

    std::string str = stream.str();
    if (!hasColons) str.erase(std::remove(str.begin(), str.end(), ':'), str.end());
    return str;
}