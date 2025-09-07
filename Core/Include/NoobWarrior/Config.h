// === noobWarrior ===
// File: Config.h
// Started by: Hattozo
// Started on: 3/8/2025
// Description:
#pragma once
#include "BaseConfig.h"

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
    Config(const std::filesystem::path &filePath, lua_State *luaState);
    ConfigResponse Open() override;
};
}