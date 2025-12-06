// === noobWarrior ===
// File: PluginManager.cpp
// Started by: Hattozo
// Started on: 12/3/2025
// Description:
#include <NoobWarrior/PluginManager.h>
#include <NoobWarrior/NoobWarrior.h>

using namespace NoobWarrior;

PluginManager::PluginManager(Core* core) : mCore(core) {}

void PluginManager::Add(Plugin *plugin, int priority) {
    mPlugins.push_back(plugin);
    plugin->Open();
}

/* NOTE: File names are relative to the path of the plugins folder in noobWarrior's user directory folder. */
void PluginManager::Add(const std::string &fileName, int priority, bool includedInInstall) {
    Add(new Plugin(fileName, mCore, includedInInstall), priority);
}

std::vector<Plugin*> PluginManager::GetPlugins() {
    return mPlugins;
}