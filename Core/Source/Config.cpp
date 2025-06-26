// === noobWarrior ===
// File: Config.cpp
// Started by: Hattozo
// Started on: 3/8/2025
// Description: Tweaks various parameters of noobWarrior functionality
#include <NoobWarrior/Config.h>
#include <NoobWarrior/DatabaseManager.h>

#include <fstream>
#include <unistd.h>

using namespace NoobWarrior;

Config::Config(const std::filesystem::path &filePath, lua_State *luaState) : BaseConfig(filePath, luaState),
    Version(NOOBWARRIOR_CONFIG_VERSION),
    Api_AssetDownload("https://assetdelivery.roblox.com/v1/asset/?id={}"),
    Api_AssetDetails("https://economy.roblox.com/v2/assets/{}/details"),
    Binaries_WineExe("wine"),
    Gui_Theme(Theme::Default)
{}

void Config::OnDeserialize() {
    DeserializeProperty<std::string>(&Api_AssetDownload, "api.asset_download");
    DeserializeProperty<std::string>(&Api_AssetDetails, "api.asset_download");

    DeserializeProperty<std::string>(&Binaries_WineExe, "binaries.wine_executable");
    DeserializeProperty<Theme>(&Gui_Theme, "gui.theme");
}

void Config::OnSerialize() {
    SerializeProperty<int>(NOOBWARRIOR_CONFIG_VERSION, "meta.version");
}
