// === noobWarrior ===
// File: Config.cpp
// Started by: Hattozo
// Started on: 3/8/2025
// Description: Tweaks various parameters of noobWarrior functionality
#include <NoobWarrior/Config.h>
#include <nlohmann/json.hpp>

#include <fstream>

#define DESERIALIZE_PROP(structProp, key) if (!key.is_null()) structProp = key;
#define SERIALIZE_PROP(structProp, key) key = structProp;

using namespace NoobWarrior;
using json = nlohmann::json;

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
    if (!std::filesystem::exists(path)) return 1; // this should still succeed even if it doesn't exist, because it's really not that important. we still have default values.
    std::ifstream fstream(path);
    if (!fstream)
        return -2;
    json data;
    try {
        data = json::parse(fstream, nullptr, true, true);
    } catch (json::exception ex) { return -1; }
    DESERIALIZE_PROP(gConfig.Api_AssetDownload, data["Api"]["AssetDownload"])
    DESERIALIZE_PROP(gConfig.Api_AssetDetails, data["Api"]["AssetDetails"])
    DESERIALIZE_PROP(gConfig.Roblox_WineExe, data["Roblox"]["WineExe"])
    // DESERIALIZE_PROP(gConfig.Gui_Theme, yamlConfig["gui"]["theme"], Theme)
    return 1;
}

int NoobWarrior::Config_WriteToFile(const std::filesystem::path &path) {
    json data;

    if (std::filesystem::exists(path)) {
        std::ifstream fstream(path);
        if (!fstream)
            return -2;
        try {
            data = json::parse(fstream, nullptr, true, true);
        } catch (json::exception ex) { return -1; }
    }
    
    data["Meta"]["FileVersion"] = NOOBWARRIOR_CONFIG_VERSION;
    SERIALIZE_PROP(gConfig.Api_AssetDownload, data["Api"]["AssetDownload"])
    SERIALIZE_PROP(gConfig.Api_AssetDetails, data["Api"]["AssetDetails"])
    SERIALIZE_PROP(gConfig.Roblox_WineExe, data["Roblox"]["WineExe"])
    // SERIALIZE_PROP(gConfig.Gui_Theme, yamlConfig["gui"]["theme"])

    std::ofstream fout(path);
    if (!fout)
        return -3;

    fout << data.dump(4);
    fout.close();

    return 1;
}

int NoobWarrior::Config_Open() {
    // return Config_ReadFromFile(GetUserDataDir().append("config.yaml"));
    return Config_ReadFromFile("config.json"); // placeholder since GetUserDataDir() currently does not work.
}

int NoobWarrior::Config_Close() {
    // std::filesystem::create_directory(GetUserDataDir());
    // return Config_WriteToFile(GetUserDataDir().append("config.yaml"));
    return Config_WriteToFile("config.json");
}