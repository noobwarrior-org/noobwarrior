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
// File: Config.h
// Started by: Hattozo
// Started on: 3/8/2025
// Description:
#pragma once
#include <NoobWarrior/BaseConfig.h>

#include <filesystem>
#include <string>
#include <vector>

#define NOOBWARRIOR_CONFIG_VERSION 1
#define NOOBWARRIOR_USERDATA_DIRNAME "noobWarrior"

namespace NoobWarrior {
class Database;

enum class Theme {
    Default = 0,
};

class Config : public BaseConfig {
public:
    Config(const std::filesystem::path &filePath, LuaState* lua);
    ConfigResponse Open() override;
};
}