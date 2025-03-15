// === noobWarrior ===
// File: Config.cpp
// Started by: Hattozo
// Started on: 3/8/2025
// Description: Tweaks various parameters of noobWarrior functionality
#include <NoobWarrior/Config.h>
#include <yaml-cpp/yaml.h>

#include <fstream>

#define DESERIALIZE_PROP(structProp, yamlNode, out) if (yamlNode) structProp = yamlNode.as<out>();
#define SERIALIZE_PROP(structProp, yamlNode) yamlNode = structProp;

using namespace NoobWarrior;

Config NoobWarrior::gConfig {};
static const Config sDefaultConfig = NoobWarrior::gConfig; // create a copy of the config

std::filesystem::path NoobWarrior::GetUserDataDir() {
// #if !defined(__APPLE__)

//     return ;
// #else
//     char path[PATH_MAX];
//     FSRef ref;
//     OSType type = kApplicationSupportFolderType;
//     OSStatus ret = FSFindFolder(kUserDomain, type, kCreateFolder, &ref); // Apparently this is deprecated, but it's the only way to do it without Objective-C so whatever.
//     FSRefMakePath(&ref, (UInt8*)&path, PATH_MAX);
//     return std::string(path) + "/noobWarrior"; // portable versions aren't allowed. cause, reasons.
// #endif
}

int NoobWarrior::Config_ReadFromFile(const std::filesystem::path &path) {
    if (!std::filesystem::exists(path)) return 0; // this should still succeed even if it doesn't exist, because it's really not that important. we still have default values.
    YAML::Node yamlConfig;
    try {
        yamlConfig = YAML::LoadFile(path);
    } catch (std::exception ex) { return -1; }
    DESERIALIZE_PROP(gConfig.Api_AssetDownload, yamlConfig["api"]["asset-download"], std::string)
    DESERIALIZE_PROP(gConfig.Api_AssetDetails, yamlConfig["api"]["asset-details"], std::string)
    DESERIALIZE_PROP(gConfig.Roblox_WineExe, yamlConfig["roblox"]["wine-executable"], std::string)
    // DESERIALIZE_PROP(gConfig.Gui_Theme, yamlConfig["gui"]["theme"], Theme)
    return 0;
}

int NoobWarrior::Config_WriteToFile(const std::filesystem::path &path) {
    std::ofstream fout(path);
    if (!fout)
        return -2;
    YAML::Node yamlConfig;
    try {
        YAML::Node yamlConfig = YAML::LoadFile(path);
    } catch (std::exception ex) { return -1; }
    yamlConfig["meta"]["file-version"] = NOOBWARRIOR_CONFIG_VERSION;
    SERIALIZE_PROP(gConfig.Api_AssetDownload, yamlConfig["api"]["asset-download"])
    SERIALIZE_PROP(gConfig.Api_AssetDownload, yamlConfig["api"]["asset-details"])
    SERIALIZE_PROP(gConfig.Roblox_WineExe, yamlConfig["roblox"]["wine-executable"])
    // SERIALIZE_PROP(gConfig.Gui_Theme, yamlConfig["gui"]["theme"])
    fout << yamlConfig;
    fout.close();
    return 0;
}

int NoobWarrior::Config_Open() {
    // return Config_ReadFromFile(GetUserDataDir().append("config.yaml"));
    return Config_ReadFromFile("config.yaml"); // placeholder since GetUserDataDir() currently does not work.
}

int NoobWarrior::Config_Close() {
    // std::filesystem::create_directory(GetUserDataDir());
    // return Config_WriteToFile(GetUserDataDir().append("config.yaml"));
    return Config_WriteToFile("config.yaml");
}