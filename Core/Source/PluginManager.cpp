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
Plugin::Response PluginManager::Mount(const std::string &fileName, int priority, bool includedInInstall) {
    Plugin* plugin = new Plugin(fileName, mCore, includedInInstall);
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
    for (Plugin::Properties prop : GetCriticalPluginProperties()) {
        if (Mount(prop.FileName, 1, true) == Plugin::Response::Success)
            loaded++;
    }

    auto selected = mCore->GetConfig()->GetKeyValue<nlohmann::json>("plugins.selected");
    if (!selected.has_value())
        return;
    
    for (auto &fileNameElement : *selected) {
        if (!fileNameElement.is_string()) continue;
        auto fileName = fileNameElement.get<std::string>();
        if (Mount(fileName) == Plugin::Response::Success)
            loaded++;
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

std::vector<Plugin*> PluginManager::GetMountedPlugins() {
    return mMountedPlugins;
}

std::vector<Plugin::Properties> PluginManager::GetAllPluginProperties() {
    std::vector<Plugin::Properties> allProps;
    std::vector<std::filesystem::path> pluginPaths;

#define ADD(dir) \
    if (std::filesystem::exists(dir / "plugins")) { \
        for (const auto &entry : std::filesystem::directory_iterator { dir / "plugins" }) { \
            std::string file_name = entry.path().filename().string(); \
            if (file_name.compare(".DS_Store") == 0) \
                continue; \
            pluginPaths.push_back(entry.path()); \
        } \
    }
    
    ADD(mCore->GetInstallationDir())
    if (!mCore->GetInit().Portable) {
        ADD(mCore->GetUserDataDir())
    }
#undef ADD

    for (std::filesystem::path path : pluginPaths) {
        std::string file_name = path.filename().string();
        
        Plugin* plugin = new Plugin(file_name, mCore, path.parent_path().parent_path() == mCore->GetInstallationDir());
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

std::vector<Plugin::Properties> PluginManager::GetCriticalPluginProperties() {
    if (!std::filesystem::exists(mCore->GetInstallationDir() / "plugins"))
        return {};

    std::vector<Plugin::Properties> allCriticalProps;
    std::vector<Plugin::Properties> allProps = GetAllPluginProperties();
    for (const auto &prop : allProps) {
        if (prop.IsCritical)
            allCriticalProps.push_back(prop);
    }

    return allCriticalProps;

    /*
    std::vector<std::string> critical_list;
    for (const auto &entry : std::filesystem::directory_iterator { mCore->GetInstallationDir() / "plugins" }) {
        const std::filesystem::path &path = entry.path();
        std::string file_name = path.filename().string();

        if (file_name.compare(".DS_Store") == 0) // why does apple do this?
            continue;

        VirtualFileSystem* vfs = nullptr;
        VirtualFileSystem::Response res = VirtualFileSystem::New(&vfs, path);
        if (res != VirtualFileSystem::Response::Success) {
            if (vfs != nullptr)
                VirtualFileSystem::Free(vfs);
            Out("PluginManager", "Failed to check if plugin \"{}\" is critical because the virtual file system failed to open.", file_name);
            continue;
        }
        
        FSEntryHandle handle = vfs->OpenHandle("/plugin.lua");
        if (handle == NULL) {
            Out("PluginManager", "Failed to check if plugin \"{}\" is critical because a handle for plugin.lua could not be opened.", file_name);
            VirtualFileSystem::Free(vfs);
            continue;
        }

        std::string pluginLuaString;
        std::string buf;
        while (vfs->ReadHandleLine(handle, &buf))
            pluginLuaString.append(buf + '\n');
        vfs->CloseHandle(handle);
        VirtualFileSystem::Free(vfs);

        lua_State* L = mCore->GetLuaState()->Get();
        int luaRet = luaL_dostring(L, pluginLuaString.c_str());
        if (luaRet != LUA_OK) {
            Out("PluginManager", "Failed to check if plugin \"{}\" is critical because plugin.lua failed: \"{}\"", file_name, lua_tostring(L, -1));
            lua_pop(L, 1);
            continue;
        }
        lua_getfield(L, -1, "critical");
        bool critical = lua_toboolean(L, -1);
        lua_pop(L, 2);

        if (critical)
            critical_list.push_back(entry.path().filename().string());
    }
    return critical_list;
    */
}
