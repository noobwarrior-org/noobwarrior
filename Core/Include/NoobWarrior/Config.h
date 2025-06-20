// === noobWarrior ===
// File: Config.h
// Started by: Hattozo
// Started on: 3/8/2025
// Description:
#pragma once

#include <filesystem>
#include <string>
#include <vector>

#define NOOBWARRIOR_CONFIG_VERSION 1
#define NOOBWARRIOR_USERDATA_DIRNAME "noobWarrior"

#if defined(_WIN32)
#define NOOBWARRIOR_USERDATA_DIR "\%localappdata%/noobWarrior/"
#elif defined(__APPLE__)
#define NOOBWARRIOR_USERDATA_DIR "~/Library/Application Support/noobWarrior/"
#elif defined(__unix__) && !defined(__APPLE__)
#define NOOBWARRIOR_USERDATA_DIR "$XDG_DATA_HOME/noobWarrior/"
#endif

namespace NoobWarrior {
    class Database;

    void SetUserDataPortable(bool val);
    void SetUserDataName(const char *name);

    std::filesystem::path GetInstallationDir();

    /**
     * @brief Warning: Any call to this function will automatically create a directory if it does not exist.
    */
    std::filesystem::path GetUserDataDir();

    enum class Theme {
        Default = 0,
    };

    struct Config {
        int Version { NOOBWARRIOR_CONFIG_VERSION };
        std::vector<std::filesystem::path> MountedArchives {};
        std::string Api_AssetDownload { "https://assetdelivery.roblox.com/v1/asset/?id={}" };
        std::string Api_AssetDetails { "https://economy.roblox.com/v2/assets/{}/details" };
        std::string Roblox_WineExe { "wine" }; // only required on non-Windows systems
        Theme Gui_Theme { Theme::Default };
    };

    extern Config gConfig;

    int Config_Open(const std::filesystem::path &path = GetUserDataDir().append("Config.json"));
    int Config_Close(const std::filesystem::path &path = GetUserDataDir().append("Config.json"));
}