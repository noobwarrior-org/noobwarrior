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
// File: Common.h
// Started by: Hattozo
// Started on: 11/11/2025
// Description: Just a few common enums and typedef aliases to help you get started.
#pragma once
#include <variant>
#include <string>
#include <vector>
#include <cstdint>

namespace NoobWarrior {
typedef std::variant<std::monostate, int, bool, int64_t, double, std::string, std::vector<unsigned char>> SqlValue;
typedef std::pair<std::string, SqlValue> SqlColumn;
typedef std::vector<SqlColumn> SqlRow;
typedef std::vector<SqlRow> SqlRows;
}