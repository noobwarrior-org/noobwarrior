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
    Plugin::Response Load(Plugin *plugin, int priority = 1);
    Plugin::Response Load(const std::string &fileName, int priority = 1, bool includedInInstall = false);
    void LoadPlugins();

    /**
     * @brief Gets all loaded plugins. This does not include plugins that are not loaded (enabled)
     */
    std::vector<Plugin*> GetPlugins();

    /**
     * @brief Gets properties of all plugins found in the install & userdata directories
     */
    std::vector<Plugin::Properties> GetAllPluginProperties();
protected:
    std::vector<Plugin::Properties> GetCriticalPluginProperties();
private:
    Core* mCore;
    std::vector<Plugin*> mPlugins;
};
}