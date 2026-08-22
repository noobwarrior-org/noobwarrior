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
// File: PluginManager.h
// Started by: Hattozo
// Started on: 12/3/2025
// Description:
#pragma once
#include <NoobWarrior/Lua/LuaState.h>
#include <NoobWarrior/Plugin.h>
#include <NoobWarrior/Url.h>

#include <string>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace NoobWarrior {
class Core;
class PluginManager {
public:
    PluginManager(Core* core);
    ~PluginManager();
    Plugin::Response Mount(Plugin* plugin, int priority = 1);
    Plugin::Response Mount(const std::filesystem::path &filePath, int priority = 1);

    void Unmount(Plugin* plugin);

    void MountPlugins();
    void UnmountPlugins();

    /**
     * @brief Runs the autorun code of every currently-mounted plugin, in the order they were
     * mounted. Called by MountPlugins() once all plugins are mounted so that plugin code can rely on
     * every other plugin already being available.
     */
    void ExecutePlugins();

    StudioServerBootstrap BuildStudioServerBootstrap(int64_t placeId, int64_t universeId);

    /**
     * @brief Returns true if a plugin with the given identifier is currently mounted.
     */
    bool IsPluginMounted(const std::string &identifier);

    /**
     * @brief Adds or removes a plugin's file name from the plugins.selected registry list. The list
     * is stored as a Lua table of file names that MountPlugins() consults on startup.
     */
    void SetPluginSelected(const std::string &fileName, bool selected);

    Plugin* GetPluginFromIdentifier(const std::string &identifier);

    /**
     * @brief Resolves the plugin that owns a plugin:// or plugindata:// url, using the url's host
     * name as the plugin identifier. Returns nullptr for any other protocol, or if no plugin with
     * that identifier is mounted.
     */
    Plugin* GetPluginFromUrl(const Url &url);

    std::vector<std::filesystem::path> GetPrivilegedPluginPaths();
    std::vector<std::filesystem::path> GetPluginPaths();

    /**
     * @brief Gets all loaded plugins. This does not include plugins that are not loaded (enabled)
     */
    std::vector<Plugin*> GetMountedPlugins();

    /**
     * @brief Gets properties of all plugins found in the install & userdata directories
     */
    std::vector<Plugin::Properties> GetAllPluginProperties();
    std::vector<Plugin::Properties> GetPluginProperties();
    std::vector<Plugin::Properties> GetPrivilegedPluginProperties();
protected:
    void MountPrivilegedPlugins();
private:
    Core* mCore;
    std::vector<Plugin*> mMountedPlugins;
};
}
