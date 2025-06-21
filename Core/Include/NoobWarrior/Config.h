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

enum class Theme {
    Default = 0,
};

class Config {
public:
    Config(bool portable = true, const std::string &fileName = "config.json");

    static std::filesystem::path GetInstallationDir();

    /**
     * @brief Warning: Any call to this function will automatically create a directory if it does not exist.
    */
    std::filesystem::path GetUserDataDir();

    int ReadFromFile();
    int WriteToFile();

    int         Version;
    std::string Api_AssetDownload;
    std::string Api_AssetDetails;
    std::string Roblox_WineExe; // only required on non-Windows systems
    Theme       Gui_Theme;
private:
    bool Portable;
    std::string FileName;
};
}