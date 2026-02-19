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

#include <lua.h>

#define ERR_LOG_TEMPLATE "Failed to load plugin \"{}\" because "
#define PLUGIN_OUT(...) Out("Plugin", "[" + identifier + "] " + __VA_ARGS__);

using namespace NoobWarrior;

/* NOTE: File names are relative to the path of the plugins folder in noobWarrior's user directory folder. */
Plugin::Plugin(const std::string &fileName, Core* core, bool includedInInstall) :
    mResponse(Response::Failed),
    mCore(core),
    mFileName(fileName),
    mVfs(nullptr),
    mIncludedInInstall(includedInInstall)
{
    std::filesystem::path fullDir = (!mIncludedInInstall ? mCore->GetUserDataDir() : mCore->GetInstallationDir()) / "plugins" / mFileName;

    if (!mCore->GetLuaState()->Opened()) {
        Out("Plugin", ERR_LOG_TEMPLATE "the Lua subsystem is not open! Perhaps the plugin was initialized too early?", mFileName);
        mResponse = Response::Failed;
        return;
    }

    // Use a virtual filesystem so that we can use both compressed archives and regular folders.
    VirtualFileSystem::Response fsRes = VirtualFileSystem::New(&mVfs, fullDir);

    if (fsRes != VirtualFileSystem::Response::Success || mVfs == nullptr) {
        Out("Plugin", ERR_LOG_TEMPLATE "the virtual filesystem failed to initialize.", mFileName);
        mResponse = Response::Failed;
        return;
    }

    if (!mVfs->EntryExists("/plugin.lua")) {
        Out("Plugin", ERR_LOG_TEMPLATE "its root directory does not contain a plugin.lua file.", mFileName);
        mResponse = Response::Failed;
        return;
    }

    std::string pluginLuaString;

    mVfsHandle = mVfs->OpenHandle("/plugin.lua");
    std::string buf;
    while (mVfs->ReadHandleLine(mVfsHandle, &buf))
        pluginLuaString.append(buf + '\n');

    lua_State *L = mCore->GetLuaState()->Get();
    int res = luaL_dostring(L, pluginLuaString.c_str());
    if (res != LUA_OK) {
        Out("Plugin", ERR_LOG_TEMPLATE "plugin.lua failed with error: {}", mFileName, lua_tostring(L, -1));
        lua_pop(L, 1);
        mResponse = Response::Failed;
        return;
    }
    if (!lua_istable(L, -1)) {
        Out("Plugin", ERR_LOG_TEMPLATE "plugin.lua did not return a string.", mFileName);
        lua_pop(L, 1);
        mResponse = Response::Failed;
        return;
    }

    lua_getfield(L, -1, "identifier");
    if (lua_isnil(L, -1)) {
        Out("Plugin", ERR_LOG_TEMPLATE "it does not have an identifier set in plugin.lua.", mFileName);
        lua_pop(L, 2);
        mResponse = Response::Failed;
        return;
    }
    lua_pop(L, 1);

    lua_getfield(L, -1, "title");
    if (lua_isnil(L, -1)) {
        Out("Plugin", ERR_LOG_TEMPLATE "it does not have a title set in plugin.lua.", mFileName);
        lua_pop(L, 2);
        mResponse = Response::Failed;
        return;
    }
    lua_pop(L, 1);

    mManifestRef = luaL_ref(L, LUA_REGISTRYINDEX);
    mResponse = Response::Success;
}

Plugin::~Plugin() {
    if (Fail())
        return;
    if (mVfs != nullptr) {
        mVfs->CloseHandle(mVfsHandle);
        VirtualFileSystem::Free(mVfs);
    }
    CloseEnv();
    luaL_unref(mCore->GetLuaState()->Get(), LUA_REGISTRYINDEX, mManifestRef);
}

Plugin::Response Plugin::Execute() {
    if (Fail()) {
        Out("Plugin", "Plugin::Execute() called but plugin \"{}\" failed to initialize!", mFileName);
        return Plugin::Response::Failed;
    }

    lua_State *L = mCore->GetLuaState()->Get();
    PushManifest();

    lua_getfield(L, -1, "identifier");
    std::string identifier = lua_tostring(L, -1);
    lua_pop(L, 1);

    PLUGIN_OUT("Executing...")

    lua_getfield(L, -1, "autorun");
    if (lua_istable(L, -1)) {
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            if (!lua_isstring(L, -1)) {
                lua_pop(L, 1);
                continue;
            }

            const char* path = lua_tostring(L, -1);
            lua_pop(L, 1); // Pop the value that lua_next just pushed.

            LuaScript script(mCore->GetLuaState(), (Url(path, {
                .DefaultProtocolType = ProtocolType::Plugin,
                .DefaultHostName = identifier
            })));
            if (!script.Fail())
                script.Execute();

            /*
            FSEntryHandle scriptHandle = mVfs->OpenHandle(path);
            
            std::string script, line;
            while (mVfs->ReadHandleLine(scriptHandle, &line)) {
                script += line + "\n";
            }

            mVfs->CloseHandle(scriptHandle);

            int compileRes = luaL_loadstring(L, script.c_str());
            if (compileRes != LUA_OK) {
                PLUGIN_OUT("({}) (Compile Failure) {}", path, lua_tostring(L, -1))
                lua_pop(L, 1);
                continue;
            }
            if (!PushEnv())
                continue;
            lua_setfenv(L, -2);

            int execRes = lua_pcall(L, 0, 0, 0);
            if (execRes != LUA_OK) {
                PLUGIN_OUT("({}) (Execution Failure) {}", path, lua_tostring(L, -1))
                lua_pop(L, 1);
                continue;
            }
            */
        }
    }
    lua_pop(L, 1);

    lua_pop(L, 1);

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

std::string Plugin::GetFileName() {
    return mFileName;
}

const Plugin::Properties Plugin::GetProperties() {
    if (Fail()) {
        Out("Plugin", "Plugin::GetProperties() called but plugin \"{}\" failed to initialize!", mFileName);
        return {};
    }
    lua_State *L = mCore->GetLuaState()->Get();
    PushManifest();

    Properties props {};
    props.FileName = mFileName;

    lua_getfield(L, -1, "identifier");
    if (lua_isstring(L, -1))
        props.Identifier = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, -1, "title");
    if (lua_isstring(L, -1))
        props.Title = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, -1, "version");
    if (lua_isstring(L, -1))
        props.Version = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, -1, "description");
    if (lua_isstring(L, -1))
        props.Description = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, -1, "critical");
    props.IsCritical = lua_toboolean(L, -1);
    lua_pop(L, 1);

    lua_pop(L, 1);
    return props;
}

void Plugin::PushManifest() {
    if (Fail())
        return;
    lua_rawgeti(mCore->GetLuaState()->Get(), LUA_REGISTRYINDEX, mManifestRef);
}

bool Plugin::PushEnv() {
    if (Fail() || mEnvRef == 0)
        return false;
    lua_State *L = mCore->GetLuaState()->Get();
    lua_rawgeti(L, LUA_REGISTRYINDEX, mEnvRef);
    return true;
}

static int plugin_print(lua_State *L) {
    std::string identifier = luaL_checkstring(L, lua_upvalueindex(1));

    int nargs = lua_gettop(L); 

    std::string msg;
    for (int i = 1; i <= nargs; i++) {
        const char *str = lua_tolstring(L, i, NULL);
        msg += str;
        lua_pop(L, 1);
    }
    PLUGIN_OUT(msg)
    return 0;
}

void Plugin::OpenEnv() {
    if (Fail())
        return;
    
    lua_State *L = mCore->GetLuaState()->Get();

    PushManifest();
    lua_getfield(L, -1, "identifier");
    const char* identifier = lua_tostring(L, -1);
    lua_pop(L, 2);

    // Environment
    lua_newtable(L);
    mEnvRef = luaL_ref(L, LUA_REGISTRYINDEX);

    PushEnv();

    PluginWrapper* plugin_wrapper = (PluginWrapper*) lua_newuserdata(L, sizeof(PluginWrapper));
    new(plugin_wrapper) PluginWrapper(this);
    luaL_setmetatable(L, "Plugin");
    lua_setfield(L, -2, "plugin");

    lua_pushstring(L, identifier);
    lua_pushcclosure(L, plugin_print, 1);
    lua_setfield(L, -2, "print");

    lua_pop(L, 1);

    /*
    // Metatable
    int res = luaL_dostring(L, plugin_env_metatable_lua);
    if (res != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        Out("Lua", "Failed to create plugin environment: {}", err);
        lua_pop(L, 1);
        return;
    }
    lua_setmetatable(L, -2);
    */
}

void Plugin::CloseEnv() {
    if (Fail() || mEnvRef == 0)
        return;
    lua_State *L = mCore->GetLuaState()->Get();
    luaL_unref(L, LUA_REGISTRYINDEX, mEnvRef);
}