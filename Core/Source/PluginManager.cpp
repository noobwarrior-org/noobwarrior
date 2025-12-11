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

void PluginManager::Load(Plugin *plugin, int priority) {
    mPlugins.push_back(plugin);
    plugin->Open();
    Out("PluginManager", "Loaded plugin \"{}\"", plugin->GetFileName());
}

/* NOTE: File names are relative to the path of the plugins folder in noobWarrior's user directory folder. */
void PluginManager::Load(const std::string &fileName, int priority, bool includedInInstall) {
    Load(new Plugin(fileName, mCore, includedInInstall), priority);
}

void PluginManager::LoadPlugins() {
    for (std::string plugin_name : GetCriticalPluginNames())
        Load(plugin_name, 1, true);

    auto selected = mCore->GetConfig()->GetKeyValue<nlohmann::json>("plugins.selected");
    if (!selected.has_value())
        return;
    for (auto &fileNameElement : *selected) {
        if (!fileNameElement.is_string()) continue;
        auto fileName = fileNameElement.get<std::string>();
        Load(fileName);
    }
}

std::vector<Plugin*> PluginManager::GetPlugins() {
    return mPlugins;
}

std::vector<std::string> PluginManager::GetCriticalPluginNames() {
    if (!std::filesystem::exists(mCore->GetInstallationDir() / "plugins"))
        return {};

    std::vector<std::string> critical_list;
    for (const auto &entry : std::filesystem::directory_iterator { mCore->GetInstallationDir() / "plugins" }) {
        const std::filesystem::path &path = entry.path();
        std::string file_name = path.filename().string();

        Out("PluginManager", "Creating vfs for plugin \"{}\"", file_name);

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

        Out("PluginManager", "Plugin string: {}", pluginLuaString);

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
}
