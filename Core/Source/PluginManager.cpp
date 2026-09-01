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
#include <NoobWarrior/EmuDb/EmuDbManager.h>
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
        mCore->Out("PluginManager", "WARNING! noobWarrior tried to unmount a plugin but the Lua subsystem is not open! Perhaps it was closed too early?");
        return;
    }

    auto it = std::find(mMountedPlugins.begin(), mMountedPlugins.end(), plugin);
    if (it != mMountedPlugins.end()) {
        std::string fileName = plugin->GetFileName();
        mMountedPlugins.erase(it);
        NOOBWARRIOR_FREE_PTR(plugin)
        mCore->Out("PluginManager", "Unmounted plugin \"{}\"", fileName);
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
        mCore->Out("PluginManager", "Loaded all enabled plugins");

    // Do not call ExecutePlugins() here. Core mounts the databases the manifests
    // declare between the two steps, because plugin code expects to find them already mounted.
}

void PluginManager::ExecutePlugins() {
    for (Plugin* plugin : mMountedPlugins)
        plugin->Execute();
}

// Turns a declared entry into a real path SQLite can open, without ever modifying the plugin.
// A writable copy is made exactly once and never refreshed afterwards, because by then it holds
// things the user generated (DataStore values, published places) that a plugin update must not wipe.
// Where a declared entry's file already is, without creating or extracting anything. Empty when
// nothing exists yet, which is the normal case for a zip plugin nobody has mounted from.
std::filesystem::path PluginManager::ResolveDeclaredDatabasePath(const Plugin::DeclaredDatabase &declared) {
    Url url(declared.SourceUrl);
    if (url.GetProtocol() != ProtocolType::Plugin) {
        std::filesystem::path resolved = url.ResolveAsLocalPath(mCore);
        return std::filesystem::exists(resolved) ? resolved : std::filesystem::path();
    }

    const std::string innerPath = url.ResolveAsPath();
    Plugin *plugin = GetPluginFromIdentifier(declared.OwnerIdentifier);
    if (plugin != nullptr && plugin->IsDirectoryBacked() && !declared.Writable) {
        std::filesystem::path inPlace = plugin->GetFilePath() / std::filesystem::path(innerPath).relative_path();
        if (std::filesystem::exists(inPlace))
            return inPlace;
    }

    // Otherwise it only exists if it has been staged before.
    std::filesystem::path staged = mCore->GetUserDataDir() / NW_PATH_PLUGINDATA /
        declared.OwnerIdentifier / "db" / std::filesystem::path(innerPath).filename();
    return std::filesystem::exists(staged) ? staged : std::filesystem::path();
}

static bool StageDatabase(Core *core, Plugin *plugin, const Plugin::DeclaredDatabase &declared,
                          std::filesystem::path *pathOutput, SqlDb::OpenMode *openModeOutput) {
    Url url(declared.SourceUrl);
    const std::string innerPath = url.ResolveAsPath();

    // Anything that is not addressing the plugin's own contents (userdata://, file://, ...) already
    // names a real file, so there is nothing to stage.
    if (url.GetProtocol() != ProtocolType::Plugin) {
        std::filesystem::path resolved = url.ResolveAsLocalPath(core);
        if (resolved.empty() || !std::filesystem::exists(resolved))
            return false;
        *pathOutput = resolved;
        *openModeOutput = declared.Writable ? SqlDb::OpenMode::ReadWrite : SqlDb::OpenMode::ReadOnly;
        return true;
    }

    if (plugin == nullptr || plugin->GetVfs() == nullptr || !plugin->GetVfs()->EntryExists(innerPath)) {
        core->Out("PluginManager", "Plugin \"{}\" declares database \"{}\" but it does not exist in the plugin",
            declared.OwnerIdentifier, declared.SourceUrl);
        return false;
    }

    if (plugin->IsDirectoryBacked() && !declared.Writable) {
        // The whole point of the read-only default: no copy, no disk cost, file untouched.
        *pathOutput = plugin->GetFilePath() / std::filesystem::path(innerPath).relative_path();
        *openModeOutput = SqlDb::OpenMode::ReadOnly;
        return true;
    }

    // Everything else needs a real file of its own in the user's data directory. SQLite cannot read
    // out of a zip at all, and a writable database must not be the plugin's own copy.
    std::filesystem::path staged = core->GetUserDataDir() / NW_PATH_PLUGINDATA /
        declared.OwnerIdentifier / "db" / std::filesystem::path(innerPath).filename();

    bool needsExtract = !std::filesystem::exists(staged);
    if (!needsExtract && !declared.Writable) {
        // Safe to refresh a read-only staged copy, since nothing the user cares about lives in it.
        std::error_code ec;
        auto stagedTime = std::filesystem::last_write_time(staged, ec);
        auto sourceTime = std::filesystem::last_write_time(plugin->GetFilePath(), ec);
        if (!ec && sourceTime > stagedTime)
            needsExtract = true;
    }

    if (needsExtract) {
        core->Out("PluginManager", "Staging database \"{}\" for plugin \"{}\"",
            std::filesystem::path(innerPath).filename().string(), declared.OwnerIdentifier);
        if (!plugin->ExtractFile(innerPath, staged)) {
            core->Out("PluginManager", "Failed to stage database \"{}\" for plugin \"{}\"",
                declared.SourceUrl, declared.OwnerIdentifier);
            return false;
        }
    }

    *pathOutput = staged;
    *openModeOutput = declared.Writable ? SqlDb::OpenMode::ReadWrite : SqlDb::OpenMode::ReadOnly;
    return true;
}

// Mounts one already-resolved entry. Shared by the required pass and the opt-in registry path.
static bool MountDeclared(Core *core, Plugin *plugin, const Plugin::DeclaredDatabase &declared,
                          unsigned int priority) {
    EmuDbManager *manager = core->GetEmuDbManager();
    if (manager->GetDbFromSourceUrl(declared.SourceUrl) != nullptr)
        return false; // already mounted

    std::filesystem::path path;
    SqlDb::OpenMode openMode = SqlDb::OpenMode::ReadOnly;
    if (!StageDatabase(core, plugin, declared, &path, &openMode))
        return false;

    // A database asking not to be written to at runtime overrides the plugin's writable request.
    if (openMode == SqlDb::OpenMode::ReadWrite && !EmuDb::ProbeIsMutable(path))
        openMode = SqlDb::OpenMode::ReadOnly;

    EmuDbManager::MountInfo info;
    info.OwnerPluginId = declared.OwnerIdentifier;
    info.SourceUrl = declared.SourceUrl;
    info.Locked = declared.Required;

    SqlDb::FailReason reason = manager->MountOwned(path, priority, info, openMode);
    if (reason != SqlDb::FailReason::None) {
        core->Out("PluginManager", "Failed to mount database \"{}\" declared by plugin \"{}\"{}",
            declared.SourceUrl, declared.OwnerIdentifier,
            reason == SqlDb::FailReason::ReadOnlyOutOfDate
                ? ": its schema is out of date and it was mounted read-only. Set writable = true in "
                  "the manifest, or open it once in noobWarrior to upgrade it."
                : "");
        return false;
    }
    return true;
}

void PluginManager::MountRequiredDatabases() {
    EmuDbManager *manager = mCore->GetEmuDbManager();
    for (Plugin *plugin : mMountedPlugins) {
        for (const Plugin::DeclaredDatabase &declared : plugin->GetDeclaredDatabases()) {
            if (!declared.Required)
                continue;
            // Append, so a plugin database can never displace the master database at index 0.
            MountDeclared(mCore, plugin, declared,
                static_cast<unsigned int>(manager->GetMountedDatabases().size()));
        }
    }
}

bool PluginManager::MountDeclaredDatabase(const std::string &sourceUrl, unsigned int priority) {
    Plugin *plugin = GetPluginFromUrl(Url(sourceUrl));
    if (plugin == nullptr) {
        mCore->Out("PluginManager", "Ignoring mounted database \"{}\": no such plugin is enabled", sourceUrl);
        return false;
    }

    for (const Plugin::DeclaredDatabase &declared : plugin->GetDeclaredDatabases()) {
        if (declared.SourceUrl != sourceUrl)
            continue;
        if (declared.Required)
            return false; // MountRequiredDatabases() owns these; do not mount one twice
        return MountDeclared(mCore, plugin, declared, priority);
    }

    mCore->Out("PluginManager", "Ignoring mounted database \"{}\": plugin \"{}\" no longer declares it",
        sourceUrl, plugin->GetIdentifier());
    return false;
}

std::vector<Plugin::DeclaredDatabase> PluginManager::GetOfferedDatabases() {
    std::vector<Plugin::DeclaredDatabase> offered;
    EmuDbManager *manager = mCore->GetEmuDbManager();
    for (Plugin *plugin : mMountedPlugins) {
        for (const Plugin::DeclaredDatabase &declared : plugin->GetDeclaredDatabases()) {
            if (declared.Required)
                continue;
            if (manager->GetDbFromSourceUrl(declared.SourceUrl) != nullptr)
                continue;
            offered.push_back(declared);
        }
    }
    return offered;
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
        mCore->Out("PluginManager", "WARNING! noobWarrior tried to unmount all plugins but the Lua subsystem is not open! Perhaps it was closed too early?");
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
        mCore->Out("PluginManager", "Failed to mount privileged plugins: loadlist.lua failed to compile: {}", err.what());
        return;
    }

    auto loadListFunc = loadListBytecodeRes.get<sol::protected_function>();
    sol::protected_function_result loadListExecRes = loadListFunc();
    if (!loadListExecRes.valid()) {
        sol::error err = loadListExecRes;
        mCore->Out("PluginManager", "Failed to mount privileged plugins: loadlist.lua failed to execute: {}", err.what());
        return;
    }

    auto loadListObj = loadListExecRes.get<sol::object>();
    if (!loadListObj.is<sol::table>()) {
        mCore->Out("PluginManager", "Failed to mount privileged plugins: loadlist.lua does not return a table!");
        return;
    }

    sol::table loadListTbl = loadListObj.as<sol::table>();
    for (int i = 1; i <= loadListTbl.size(); i++) {
        auto privPluginObj = loadListTbl.get<sol::object>(i);
        if (!privPluginObj.is<std::string>()) {
            mCore->Out("PluginManager", "Value in index {} in loadlist.lua is not string!", i);
            continue;
        }
        auto privPluginName = privPluginObj.as<std::string>();
        mCore->Out("PluginManager", "Mounting privileged plugin \"{}\"", privPluginName);
        if (Mount(privPluginsPath / privPluginName) == Plugin::Response::Success)
            loaded++;
    }
}
