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
#define NOOBWARRIOR_USERDATA_DIRNAME "noobwarrior"

namespace NoobWarrior {
class Database;

enum class Theme {
    Default = 0,
};

class Config : public BaseConfig {
public:
    Config(const std::filesystem::path &filePath, lua_State *luaState);

    int         Version;
    std::string Api_AssetDownload;
    std::string Api_AssetDetails;
    std::string Binaries_WineExe; // only required on non-Windows systems
    Theme       Gui_Theme;
private:
    void OnDeserialize() override;
    void OnSerialize() override;
};
}