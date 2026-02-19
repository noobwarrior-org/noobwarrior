/*
 * Copyright (C) 2026 Hattozo
 *
 * This file is part of noobWarrior.
 *
 * noobWarrior is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * noobWarrior is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with noobWarrior; if not, see
 * <https://www.gnu.org/licenses/>.
 */
// === noobWarrior ===
// File: PluginManager.cpp
// Started by: Hattozo
// Started on: 12/3/2025
// Description:
#include <NoobWarrior/PluginManager.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/FileSystem/VirtualFileSystem.h>
#include <NoobWarrior/Paths.h>

using namespace NoobWarrior;

PluginManager::PluginManager(Core* core) : mCore(core) {}

PluginManager::~PluginManager() {}

Plugin::Response PluginManager::Mount(Plugin *plugin, int priority) {
    mMountedPlugins.push_back(plugin);
    Plugin::Response res = plugin->GetInitResponse();
    if (res != Plugin::Response::Success) {
        mMountedPlugins.pop_back();
        return res;
    }
    Plugin::Response execRes = plugin->Execute();
    if (res != Plugin::Response::Success)
        mMountedPlugins.pop_back();
    return execRes;
}

/* NOTE: File names are relative to the path of the plugins folder in noobWarrior's user directory folder. */
Plugin::Response PluginManager::Mount(const std::filesystem::path &filePath, int priority) {
    Plugin* plugin = new Plugin(filePath, mCore);
    Plugin::Response res = Mount(plugin, priority);
    if (res != Plugin::Response::Success) {
        NOOBWARRIOR_FREE_PTR(plugin)
    }
    return res;
}

void PluginManager::Unmount(Plugin* plugin) {
    if (!mCore->GetLuaState()->Opened()) {
        Out("PluginManager", "WARNING! noobWarrior tried to unmount a plugin but the Lua subsystem is not open! Perhaps it was closed too early?");
        return;
    }

    auto it = std::find(mMountedPlugins.begin(), mMountedPlugins.end(), plugin);
    if (it != mMountedPlugins.end()) {
        std::string fileName = plugin->GetFileName();
        mMountedPlugins.erase(it);
        NOOBWARRIOR_FREE_PTR(plugin)
        Out("PluginManager", "Unmounted plugin \"{}\"", fileName);
    }
}

void PluginManager::MountPlugins() {
    int loaded = 0;
    for (const std::filesystem::path &path : GetPrivilegedPluginPaths()) {
        if (Mount(path, 0) == Plugin::Response::Success)
            loaded++;
    }

    auto selected = mCore->GetConfig()->GetKeyValue<nlohmann::json>("plugins.selected");
    if (!selected.has_value())
        return;
    
    for (auto &fileNameElement : *selected) {
        if (!fileNameElement.is_string()) continue;
        auto fileName = fileNameElement.get<std::string>();

        std::filesystem::path installPath = mCore->GetInstallationDir() / NW_PATH_PLUGINS / fileName;
        std::filesystem::path userPath = mCore->GetUserDataDir() / NW_PATH_PLUGINS / fileName;

        if (std::filesystem::exists(userPath) && Mount(userPath) == Plugin::Response::Success) {
            // Plugins in the user data folder take higher priority, in order to foster modding more easily
            loaded++;
            continue;
        }

        if (std::filesystem::exists(installPath) && Mount(installPath) == Plugin::Response::Success) {
            loaded++;
            continue;
        }
    }
    
    if (loaded > 0)
        Out("PluginManager", "Loaded all enabled plugins");
}

void PluginManager::UnmountPlugins() {
    if (!mCore->GetLuaState()->Opened()) {
        Out("PluginManager", "WARNING! noobWarrior tried to unmount all plugins but the Lua subsystem is not open! Perhaps it was closed too early?");
        return;
    }
    for (Plugin* plugin : mMountedPlugins) {
        Unmount(plugin);
    }
}

Plugin* PluginManager::GetPluginFromIdentifier(const std::string &identifier) {
    for (Plugin *plugin : GetMountedPlugins()) {
        Plugin::Properties props = plugin->GetProperties();
        if (identifier.compare(props.Identifier) == 0)
            return plugin;
    }
    return nullptr;
}

static std::vector<std::filesystem::path> GetEntriesInDir(const std::filesystem::path &path) {
    std::vector<std::filesystem::path> paths;
    if (!std::filesystem::exists(path))
        return paths;
    for (const auto &entry : std::filesystem::directory_iterator { path }) {
        std::string file_name = entry.path().filename().string();
        if (file_name.compare(".DS_Store") == 0)
            continue;
        paths.push_back(entry.path());
    }
    return paths;
}

std::vector<std::filesystem::path> PluginManager::GetPrivilegedPluginPaths() {
    return GetEntriesInDir(mCore->GetInstallationDir() / NW_PATH_PRIVILEGED_PLUGINS);
}

std::vector<std::filesystem::path> PluginManager::GetPluginPaths() {
    auto installEntries = GetEntriesInDir(mCore->GetInstallationDir() / NW_PATH_PLUGINS);
    auto userEntries = GetEntriesInDir(mCore->GetUserDataDir() / NW_PATH_PLUGINS);
    std::vector<std::filesystem::path> paths;
    paths.insert(paths.end(), installEntries.begin(), installEntries.end());
    paths.insert(paths.end(), userEntries.begin(), userEntries.end());
    return paths;
}

std::vector<Plugin*> PluginManager::GetMountedPlugins() {
    return mMountedPlugins;
}

std::vector<Plugin::Properties> PluginManager::GetAllPluginProperties() {
    std::vector<Plugin::Properties> allProps;

    auto privPluginPaths = GetPrivilegedPluginPaths();
    auto pluginPaths = GetPluginPaths();
    std::vector<std::filesystem::path> allPluginPaths;
    allPluginPaths.insert(allPluginPaths.end(), privPluginPaths.begin(), privPluginPaths.end());
    allPluginPaths.insert(allPluginPaths.end(), pluginPaths.begin(), pluginPaths.end());

    for (std::filesystem::path path : allPluginPaths) {
        std::string file_name = path.filename().string();
        
        Plugin* plugin = new Plugin(file_name, mCore);
        if (plugin->Fail()) {
            NOOBWARRIOR_FREE_PTR(plugin)
            continue;
        }

        Plugin::Properties props = plugin->GetProperties();
        allProps.push_back(props);

        NOOBWARRIOR_FREE_PTR(plugin)
    }
    return allProps;
}

std::vector<Plugin::Properties> PluginManager::GetPrivilegedPluginProperties() {
    std::vector<Plugin::Properties> allCriticalProps;
    std::vector<Plugin::Properties> allProps = GetAllPluginProperties();
    for (const auto &prop : allProps) {
        if (prop.IsPrivileged)
            allCriticalProps.push_back(prop);
    }

    return allCriticalProps;
}
