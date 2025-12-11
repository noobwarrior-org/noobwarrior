// === noobWarrior ===
// File: PluginManager.h
// Started by: Hattozo
// Started on: 12/3/2025
// Description:
#pragma once
#include <NoobWarrior/LuaState.h>
#include <NoobWarrior/Plugin.h>

#include <string>
#include <filesystem>
#include <vector>

namespace NoobWarrior {
class Core;
class PluginManager {
public:
    PluginManager(Core* core);
    void Load(Plugin *plugin, int priority = 1);
    void Load(const std::string &fileName, int priority = 1, bool includedInInstall = false);
    void LoadPlugins();

    std::vector<Plugin*> GetPlugins();
protected:
    std::vector<std::string> GetCriticalPluginNames();
private:
    Core* mCore;
    std::vector<Plugin*> mPlugins;
};
}