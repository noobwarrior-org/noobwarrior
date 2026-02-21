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
// File: Plugin.cpp
// Started by: Hattozo
// Started on: 12/3/2025
// Description:
#include <NoobWarrior/Plugin.h>
#include <NoobWarrior/Lua/Bridge/PluginBridge.h>
#include <NoobWarrior/Lua/LuaState.h>
#include <NoobWarrior/Lua/LuaScript.h>
#include <NoobWarrior/Url.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/FileSystem/VirtualFileSystem.h>

#include "Lua/files/plugin_env_metatable.lua.inc.cpp"

#include <lua.hpp>
#include <sol/sol.hpp>

#define ERR_LOG_TEMPLATE "Failed to load plugin \"{}\" because "
#define PLUGIN_OUT(...) Out("Plugin", "[" + identifier + "] " + __VA_ARGS__);

using namespace NoobWarrior;

/* NOTE: File names are relative to the path of the plugins folder in noobWarrior's user directory folder. */
Plugin::Plugin(const std::filesystem::path &filePath, Core* core) :
    mResponse(Response::Failed),
    mCore(core),
    mFilePath(filePath),
    mVfs(nullptr)
{
    if (!mCore->GetLuaState()->Opened()) {
        Out("Plugin", ERR_LOG_TEMPLATE "the Lua subsystem is not open! Perhaps the plugin was initialized too early?", GetFileName());
        mResponse = Response::Failed;
        return;
    }

    // Use a virtual filesystem so that we can use both compressed archives and regular folders.
    VirtualFileSystem::Response fsRes = VirtualFileSystem::New(&mVfs, mFilePath);
    if (fsRes != VirtualFileSystem::Response::Success || mVfs == nullptr) {
        Out("Plugin", ERR_LOG_TEMPLATE "the virtual filesystem failed to initialize.", GetFileName());
        mResponse = Response::Failed;
        return;
    }

    if (!mVfs->EntryExists("/plugin.lua")) {
        Out("Plugin", ERR_LOG_TEMPLATE "its root directory does not contain a plugin.lua file.", GetFileName());
        mResponse = Response::Failed;
        return;
    }

    std::string pluginLuaString;

    mVfsHandle = mVfs->OpenHandle("/plugin.lua");
    std::string buf;
    while (mVfs->ReadHandleLine(mVfsHandle, &buf))
        pluginLuaString.append(buf + '\n');

    sol::protected_function_result res = mCore->GetLuaState()->safe_script(pluginLuaString);
    if (!res.valid()) {
        sol::error err = res;
        Out("Plugin", ERR_LOG_TEMPLATE "plugin.lua failed with error: {}", GetFileName(), err.what());
        mResponse = Response::Failed;
        return;
    }
    if (res.get_type() != sol::type::table) {
        Out("Plugin", ERR_LOG_TEMPLATE "plugin.lua did not return a table.", GetFileName());
        mResponse = Response::Failed;
        return;
    }

    mManifestTbl = res.get<sol::table>();
    auto identifier = mManifestTbl.get<std::optional<std::string>>("identifier");
    auto title = mManifestTbl.get<std::optional<std::string>>("title");

    if (identifier == std::nullopt) {
        Out("Plugin", ERR_LOG_TEMPLATE "it does not have an identifier set in plugin.lua.", GetFileName());
        mResponse = Response::Failed;
        return;
    }

    if (title == std::nullopt) {
        Out("Plugin", ERR_LOG_TEMPLATE "it does not have a title set in plugin.lua.", GetFileName());
        mResponse = Response::Failed;
        return;
    }

    mResponse = Response::Success;
    OpenEnv();
}

Plugin::~Plugin() {
    if (Fail())
        return;
    if (mVfs != nullptr) {
        mVfs->CloseHandle(mVfsHandle);
        VirtualFileSystem::Free(mVfs);
    }
}

Plugin::Response Plugin::Execute() {
    if (Fail()) {
        Out("Plugin", "Plugin::Execute() called but plugin \"{}\" failed to initialize!", GetFileName());
        return Plugin::Response::Failed;
    }

    auto identifier = mManifestTbl.get<std::string>("identifier");

    PLUGIN_OUT("Executing...")

    auto autorunTbl = mManifestTbl.get<std::optional<sol::table>>("autorun");
    if (autorunTbl != std::nullopt) {
        for (int i = 1; i <= autorunTbl->size(); i++) {
            auto val = autorunTbl->get<sol::object>(i);
            if (!val.is<std::string>()) {
                PLUGIN_OUT("Value in index {} in autorun is not string!", i)
                continue;
            }
            LuaScript script(mCore->GetLuaState(), mEnv, (Url(val.as<std::string>(), {
                .DefaultProtocolType = ProtocolType::Plugin,
                .DefaultHostName = identifier
            })));
            if (!script.Fail())
                script.Execute();
        }
    }

    return Response::Success;
}

bool Plugin::Fail() {
    return mResponse != Response::Success;
}

Plugin::Response Plugin::GetInitResponse() {
    return mResponse;
}

VirtualFileSystem* Plugin::GetVfs() {
    return mVfs;
}

std::filesystem::path Plugin::GetFilePath() {
    return mFilePath;
}

std::string Plugin::GetFileName() {
    std::string str = mFilePath.string();
    std::string::size_type last_slash = str.find_last_of("/");
	return last_slash != std::string::npos ? str.substr(last_slash + 1) : str;
}

std::string Plugin::GetIdentifier() {
    return mManifestTbl.get_or<std::string>("identifier", "");
}

const Plugin::Properties Plugin::GetProperties() {
    if (Fail()) {
        Out("Plugin", "Plugin::GetProperties() called but plugin \"{}\" failed to initialize!", GetFileName());
        return {};
    }

    Properties props {};
    props.FilePath = mFilePath;
    props.FileName = GetFileName();

    props.Identifier = mManifestTbl.get_or<std::string>("identifier", "");
    props.Title = mManifestTbl.get_or<std::string>("title", "");
    props.Version = mManifestTbl.get_or<std::string>("version", "");
    props.Description = mManifestTbl.get_or<std::string>("description", "");

    props.IsPrivileged = mFilePath.parent_path().filename().compare("priv-plugins") == 0;

    return props;
}

void Plugin::OpenEnv() {
    if (Fail())
        return;

    auto identifier = mManifestTbl.get<std::string>("identifier");

    mEnv = sol::environment(*mCore->GetLuaState(), sol::create, mCore->GetLuaState()->globals());
    mEnv.set("plugin", this);
}
