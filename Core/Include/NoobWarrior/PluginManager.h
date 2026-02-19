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

#include <string>
#include <filesystem>
#include <vector>

namespace NoobWarrior {
class Core;
class PluginManager {
public:
    PluginManager(Core* core);
    ~PluginManager();
    Plugin::Response Mount(Plugin* plugin, int priority = 1);
    Plugin::Response Mount(const std::string &fileName, int priority = 1, bool includedInInstall = false);

    void Unmount(Plugin* plugin);

    void MountPlugins();
    void UnmountPlugins();

    Plugin* GetPluginFromIdentifier(const std::string &identifier);

    /**
     * @brief Gets all loaded plugins. This does not include plugins that are not loaded (enabled)
     */
    std::vector<Plugin*> GetMountedPlugins();

    /**
     * @brief Gets properties of all plugins found in the install & userdata directories
     */
    std::vector<Plugin::Properties> GetAllPluginProperties();
protected:
    std::vector<Plugin::Properties> GetCriticalPluginProperties();
private:
    Core* mCore;
    std::vector<Plugin*> mMountedPlugins;
};
}