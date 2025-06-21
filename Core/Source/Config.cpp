// === noobWarrior ===
// File: Config.cpp
// Started by: Hattozo
// Started on: 3/8/2025
// Description: Tweaks various parameters of noobWarrior functionality
#include <NoobWarrior/Config.h>
#include <NoobWarrior/DatabaseManager.h>
#include <nlohmann/json.hpp>

#include <fstream>

#if defined(_WIN32)
#include <windows.h>
#include <shlobj.h>
#endif

#define DESERIALIZE_PROP(structProp, key) if (!key.is_null()) structProp = key;
#define SERIALIZE_PROP(structProp, key) key = structProp;

using namespace NoobWarrior;
using json = nlohmann::json;

std::filesystem::path Config::GetInstallationDir() {
#if defined(_WIN32)
    WCHAR buf[MAX_PATH];
    GetModuleFileNameW(NULL, buf, MAX_PATH);
    return std::filesystem::path(buf).parent_path();
#endif
}

Config::Config(bool portable, const std::string &fileName) :
    Version(NOOBWARRIOR_CONFIG_VERSION),
    Api_AssetDownload("https://assetdelivery.roblox.com/v1/asset/?id={}"),
    Api_AssetDetails("https://economy.roblox.com/v2/assets/{}/details"),
    Roblox_WineExe("wine"),
    Gui_Theme(Theme::Default),

    Portable(portable),
    FileName(fileName)
{}

std::filesystem::path Config::GetUserDataDir() {
#if defined(_WIN32)
    if (!Portable) {
        // Our user data directory is in the Documents folder instead of the %LocalAppData% directory, which is where most developers will store their user data.
        // We do this because Windows wipes away your AppData folder when you reinstall the OS, even when you choose to keep your data.
        // Typically it won't actually do this and it will instead move it away to a Windows.old folder
        // But, most people do not realize this and they end up losing everything when the OS decides to delete the directory after a grace period.
        WCHAR *path;
        SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_DEFAULT, NULL, &path);
        std::filesystem::path documentsDir(path);
        std::filesystem::create_directory(documentsDir.append(NOOBWARRIOR_USERDATA_DIRNAME));
        CoTaskMemFree(path);
        return documentsDir.append(NOOBWARRIOR_USERDATA_DIRNAME);
    }
#endif
    return GetInstallationDir();
}

int Config::ReadFromFile() {
    if (!std::filesystem::exists(GetUserDataDir() / FileName)) return 1; // this should still succeed even if it doesn't exist, because it's really not that important. we still have default values.
    std::ifstream fstream(GetUserDataDir() / FileName);
    if (!fstream)
        return -2;
    json data;
    try {
        data = json::parse(fstream, nullptr, true, true);
    } catch (json::exception ex) { return -1; }
    // DESERIALIZE_PROP(MountedArchives, data["MountedArchives"])
    DESERIALIZE_PROP(Api_AssetDownload, data["Api"]["AssetDownload"])
    DESERIALIZE_PROP(Api_AssetDetails, data["Api"]["AssetDetails"])
    DESERIALIZE_PROP(Roblox_WineExe, data["Roblox"]["WineExe"])
    DESERIALIZE_PROP(Gui_Theme, data["Gui"]["Theme"])
    return 1;
}

int Config::WriteToFile() {
    json data;

    if (std::filesystem::exists(GetUserDataDir() / FileName)) {
        std::ifstream fstream(GetUserDataDir() / FileName);
        if (!fstream)
            return -2;
        try {
            data = json::parse(fstream, nullptr, true, true);
        } catch (json::exception ex) { return -1; }
    }
    
    data["Meta"]["FileVersion"] = NOOBWARRIOR_CONFIG_VERSION;
    // SERIALIZE_PROP(MountedArchives, data["MountedArchives"])
    SERIALIZE_PROP(Api_AssetDownload, data["Api"]["AssetDownload"])
    SERIALIZE_PROP(Api_AssetDetails, data["Api"]["AssetDetails"])
    SERIALIZE_PROP(Roblox_WineExe, data["Roblox"]["WineExe"])
    SERIALIZE_PROP(Gui_Theme, data["Gui"]["Theme"])

    // for (int i = 0; i < gConfig.MountedArchives.size(); i++)
        // ArchiveManager::AddArchive(gConfig.MountedArchives.at(i), i);

    std::ofstream fout(GetUserDataDir() / FileName);
    if (!fout)
        return -3;

    fout << data.dump(4);
    fout.close();

    return 1;
}
