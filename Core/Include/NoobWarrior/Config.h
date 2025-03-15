// === noobWarrior ===
// File: Config.h
// Started by: Hattozo
// Started on: 3/8/2025
// Description:
#pragma once

#include <filesystem>
#include <string>

#define NOOBWARRIOR_CONFIG_VERSION 1

#if defined(_WIN32)
#define NOOBWARRIOR_USERDATA_DIR "\%localappdata%/noobWarrior/"
#elif defined(__APPLE__)
#define NOOBWARRIOR_USERDATA_DIR "~/Library/Application Support/noobWarrior/"
#elif defined(__unix__) && !defined(__APPLE__)
#define NOOBWARRIOR_USERDATA_DIR "$XDG_DATA_HOME/noobWarrior/"
#endif

namespace NoobWarrior {
    std::filesystem::path GetInstallationDir();
    std::filesystem::path GetUserDataDir();

    enum class Theme {
        Default = 0,
    };
    struct Config {
        int Version { NOOBWARRIOR_CONFIG_VERSION };
        std::string Api_AssetDownload { "https://assetdelivery.roblox.com/v1/asset/?id={}" };
        std::string Api_AssetDetails { "https://economy.roblox.com/v2/assets/{}/details" };
        std::string Roblox_WineExe { "wine" }; // only required on non-Windows systems
        Theme Gui_Theme { Theme::Default };
    };

    extern Config gConfig;

    // these require you to specify the paths yourself
    int Config_ReadFromFile(const std::filesystem::path &path);
    int Config_WriteToFile(const std::filesystem::path &path);

    // these automatically handle which directory to put the config file in for you.
    int Config_Open();
    int Config_Close();
}