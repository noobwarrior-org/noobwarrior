// === noobWarrior ===
// File: Config.h
// Started by: Hattozo
// Started on: 3/8/2025
// Description:
#pragma once

#include <filesystem>
#include <string_view>

#define NOOBWARRIOR_CONFIG_VERSION 1

#if defined(_WIN32)
#define NOOBWARRIOR_USERDATA_DIR "\%localappdata%/noobWarrior/"
#elif defined(__APPLE__)
#define NOOBWARRIOR_USERDATA_DIR "~/Library/Application Support/noobWarrior/"
#elif defined(__unix__) && !defined(__APPLE__)
#define NOOBWARRIOR_USERDATA_DIR "$XDG_DATA_HOME/noobWarrior/"
#endif

namespace NoobWarrior {
    std::filesystem::path GetUserDataDir();

    enum class Theme {
        Default = 0,
    };
    typedef struct {
        int Version;
        std::string Api_AssetDownload;
        std::string Api_AssetDetails;
        std::string Roblox_WineExe; // only required on non-Windows systems
        Theme Gui_Theme;
    } config_t;

    extern config_t gConfig;

    // these require you to specify the paths yourself
    int Config_ReadFromFile(const std::filesystem::path &path);
    int Config_WriteToFile(const std::filesystem::path &path);

    // these automatically handle which directory to put the config file in for you.
    int Config_Open();
    int Config_Close();
}