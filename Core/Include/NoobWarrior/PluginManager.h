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
    void Add(Plugin *plugin, int priority = 1);
    void Add(const std::string &fileName, int priority = 1, bool includedInInstall = false);

    std::vector<Plugin*> GetPlugins();
private:
    Core* mCore;
    std::vector<Plugin*> mPlugins;
};
}