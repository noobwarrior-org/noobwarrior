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

#include <iterator>

using namespace NoobWarrior;

PluginManager::PluginManager(Core* core) : mCore(core) {}

PluginManager::~PluginManager() {}

Plugin::Response PluginManager::Mount(Plugin *plugin, int priority) {
    Plugin::Response res = plugin->GetInitResponse();
    if (res != Plugin::Response::Success)
        return res;
    mMountedPlugins.push_back(plugin);
    return res;
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

bool PluginManager::IsPluginMounted(const std::string &identifier) {
    return GetPluginFromIdentifier(identifier) != nullptr;
}

namespace {
// plugins.selected is an array, and its order is the mount order: plugins are applied in turn and
// a later one overwrites what an earlier one placed. Iterating a sol::table with a range-for walks
// it in hash order, so the list has to be read by index or the user's chosen order is lost.
std::vector<std::string> ReadSelectedPluginFileNames(Core *core) {
    std::vector<std::string> fileNames;
    auto selected = core->GetRegistry()->GetKeyValue<sol::table>("plugins.selected");
    if (!selected.has_value())
        return fileNames;
    const std::size_t count = selected->size();
    fileNames.reserve(count);
    for (std::size_t index = 1; index <= count; ++index) {
        sol::optional<std::string> name = selected->get<sol::optional<std::string>>(index);
        if (name.has_value() && !name->empty())
            fileNames.push_back(*name);
    }
    return fileNames;
}
} // namespace

void PluginManager::SetPluginSelected(const std::string &fileName, bool selected) {
    // Read the current list of selected plugin file names, dropping any existing occurrence of
    // fileName so we don't create duplicates when re-enabling. Everything else keeps its place:
    // toggling one plugin must not reorder the others.
    std::vector<std::string> fileNames;
    for (std::string &name : ReadSelectedPluginFileNames(mCore)) {
        if (name != fileName)
            fileNames.push_back(std::move(name));
    }
    // A newly enabled plugin mounts last, so it wins against everything already in the list.
    if (selected)
        fileNames.push_back(fileName);

    sol::table tbl = mCore->GetLuaState()->create_table();;
    for (std::size_t i = 0; i < fileNames.size(); i++)
        tbl.add(fileNames[i]);

    mCore->GetRegistry()->SetKeyValue<sol::table>("plugins.selected", tbl);
}

void PluginManager::MountPlugins() {
    int loaded = 0;

    MountPrivilegedPlugins();
    /*
    for (const std::filesystem::path &path : GetPrivilegedPluginPaths()) {
        if (Mount(path, 0) == Plugin::Response::Success)
            loaded++;
    }
    */

    for (const std::string &fileName : ReadSelectedPluginFileNames(mCore)) {
        std::filesystem::path installPath = mCore->GetInstallDataDir() / NW_PATH_PLUGINS / fileName;
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

    ExecutePlugins();
}

void PluginManager::ExecutePlugins() {
    for (Plugin* plugin : mMountedPlugins)
        plugin->Execute();
}

StudioServerBootstrap PluginManager::BuildStudioServerBootstrap(int64_t placeId, int64_t universeId) {
    StudioServerBootstrap bootstrap;
    for (Plugin *plugin : mMountedPlugins) {
        StudioServerBootstrap built = plugin->BuildStudioServerBootstrap(placeId, universeId);
        bootstrap.Plans.insert(bootstrap.Plans.end(),
            built.Plans.begin(), built.Plans.end());
        bootstrap.Scripts.insert(bootstrap.Scripts.end(),
            std::make_move_iterator(built.Scripts.begin()),
            std::make_move_iterator(built.Scripts.end()));
        bootstrap.Models.insert(bootstrap.Models.end(),
            std::make_move_iterator(built.Models.begin()),
            std::make_move_iterator(built.Models.end()));
    }
    return bootstrap;
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
        if (identifier.compare(plugin->GetIdentifier()) == 0)
            return plugin;
    }
    return nullptr;
}

Plugin* PluginManager::GetPluginFromUrl(const Url &url) {
    if (url.GetProtocol() != ProtocolType::Plugin && url.GetProtocol() != ProtocolType::PluginData)
        return nullptr;
    if (url.GetHostName().empty())
        return nullptr;
    return GetPluginFromIdentifier(url.GetHostName());
}

static std::vector<std::filesystem::path> GetEntriesInDir(const std::filesystem::path &path) {
    std::vector<std::filesystem::path> paths;
    if (!std::filesystem::exists(path))
        return paths;
    for (const auto &entry : std::filesystem::directory_iterator { path }) {
        std::string file_name = entry.path().filename().string();
        if (file_name.compare(".DS_Store") == 0)
            continue;
        if (file_name.compare("loadlist.lua") == 0)
            continue;
        paths.push_back(entry.path());
    }
    return paths;
}

std::vector<std::filesystem::path> PluginManager::GetPrivilegedPluginPaths() {
    return GetEntriesInDir(mCore->GetInstallDataDir() / NW_PATH_PRIVILEGED_PLUGINS);
}

std::vector<std::filesystem::path> PluginManager::GetPluginPaths() {
    auto installEntries = GetEntriesInDir(mCore->GetInstallDataDir() / NW_PATH_PLUGINS);
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
        Plugin* plugin = new Plugin(path, mCore);
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

std::vector<Plugin::Properties> PluginManager::GetPluginProperties() {
    std::vector<Plugin::Properties> props;
    std::vector<Plugin::Properties> allProps = GetAllPluginProperties();
    for (const auto &prop : allProps) {
        if (!prop.IsPrivileged)
            props.push_back(prop);
    }

    return props;
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

void PluginManager::MountPrivilegedPlugins() {
    int loaded = 0;

    std::filesystem::path privPluginsPath = mCore->GetInstallDataDir() / NW_PATH_PRIVILEGED_PLUGINS;
    std::filesystem::path loadListPath = privPluginsPath / "loadlist.lua";
    if (!std::filesystem::exists(loadListPath)) {
        return;
    }

    sol::load_result loadListBytecodeRes = mCore->GetLuaState()->load_file(loadListPath.string());
    if (!loadListBytecodeRes.valid()) {
        sol::error err = loadListBytecodeRes;
        Out("PluginManager", "Failed to mount privileged plugins: loadlist.lua failed to compile: {}", err.what());
        return;
    }

    auto loadListFunc = loadListBytecodeRes.get<sol::protected_function>();
    sol::protected_function_result loadListExecRes = loadListFunc();
    if (!loadListExecRes.valid()) {
        sol::error err = loadListExecRes;
        Out("PluginManager", "Failed to mount privileged plugins: loadlist.lua failed to execute: {}", err.what());
        return;
    }

    auto loadListObj = loadListExecRes.get<sol::object>();
    if (!loadListObj.is<sol::table>()) {
        Out("PluginManager", "Failed to mount privileged plugins: loadlist.lua does not return a table!");
        return;
    }

    sol::table loadListTbl = loadListObj.as<sol::table>();
    for (int i = 1; i <= loadListTbl.size(); i++) {
        auto privPluginObj = loadListTbl.get<sol::object>(i);
        if (!privPluginObj.is<std::string>()) {
            Out("PluginManager", "Value in index {} in loadlist.lua is not string!", i);
            continue;
        }
        auto privPluginName = privPluginObj.as<std::string>();
        Out("PluginManager", "Mounting privileged plugin \"{}\"", privPluginName);
        if (Mount(privPluginsPath / privPluginName) == Plugin::Response::Success)
            loaded++;
    }
}
