// === noobWarrior ===
// File: Config.cpp
// Started by: Hattozo
// Started on: 3/8/2025
// Description: Tweaks various parameters of noobWarrior functionality
#include <NoobWarrior/Config.h>
#include <NoobWarrior/ArchiveManager.h>
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

Config NoobWarrior::gConfig {};
static const Config sDefaultConfig = NoobWarrior::gConfig; // create a copy of the config

std::filesystem::path NoobWarrior::GetInstallationDir() {
#if defined(_WIN32)
    WCHAR buf[MAX_PATH];
    GetModuleFileNameW(NULL, buf, MAX_PATH);
    return std::filesystem::path(buf).parent_path();
#endif
}

std::filesystem::path NoobWarrior::GetUserDataDir() {
#if defined(_WIN32)
    if (!std::filesystem::exists(GetInstallationDir().append("NW_PORTABLE"))) {
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
    } else {
        return GetInstallationDir();
    }
#endif
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

int NoobWarrior::Config_Open(const std::filesystem::path &path) {
    if (!std::filesystem::exists(path)) return 1; // this should still succeed even if it doesn't exist, because it's really not that important. we still have default values.
    std::ifstream fstream(path);
    if (!fstream)
        return -2;
    json data;
    try {
        data = json::parse(fstream, nullptr, true, true);
    } catch (json::exception ex) { return -1; }
    DESERIALIZE_PROP(gConfig.MountedArchives, data["MountedArchives"])
    DESERIALIZE_PROP(gConfig.Api_AssetDownload, data["Api"]["AssetDownload"])
    DESERIALIZE_PROP(gConfig.Api_AssetDetails, data["Api"]["AssetDetails"])
    DESERIALIZE_PROP(gConfig.Roblox_WineExe, data["Roblox"]["WineExe"])
    DESERIALIZE_PROP(gConfig.Gui_Theme, data["Gui"]["Theme"])
    return 1;
}

int NoobWarrior::Config_Close(const std::filesystem::path &path) {
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
    SERIALIZE_PROP(gConfig.MountedArchives, data["MountedArchives"])
    SERIALIZE_PROP(gConfig.Api_AssetDownload, data["Api"]["AssetDownload"])
    SERIALIZE_PROP(gConfig.Api_AssetDetails, data["Api"]["AssetDetails"])
    SERIALIZE_PROP(gConfig.Roblox_WineExe, data["Roblox"]["WineExe"])
    SERIALIZE_PROP(gConfig.Gui_Theme, data["Gui"]["Theme"])

    for (int i = 0; i < gConfig.MountedArchives.size(); i++)
        ArchiveManager::AddArchive(gConfig.MountedArchives.at(i), i);

    std::ofstream fout(path);
    if (!fout)
        return -3;

    fout << data.dump(4);
    fout.close();

    return 1;
}
