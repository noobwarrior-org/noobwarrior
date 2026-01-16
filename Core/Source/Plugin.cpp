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
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/FileSystem/VirtualFileSystem.h>

#include <lua.h>

#define ERR_LOG_TEMPLATE "Failed to load plugin \"{}\" because "

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

    mRef = luaL_ref(L, LUA_REGISTRYINDEX);
    mResponse = Response::Success;
}

Plugin::~Plugin() {
    if (Fail())
        return;
    if (mVfs != nullptr) {
        mVfs->CloseHandle(mVfsHandle);
        VirtualFileSystem::Free(mVfs);
    }
    luaL_unref(mCore->GetLuaState()->Get(), LUA_REGISTRYINDEX, mRef);
}

Plugin::Response Plugin::Execute() {
    if (Fail()) {
        Out("Plugin", "Plugin::Execute() called but plugin \"{}\" failed to initialize!", mFileName);
        return Plugin::Response::Failed;
    }
    Out("Plugin", "Executing \"{}\"", mFileName);

    lua_State *L = mCore->GetLuaState()->Get();
    lua_rawgeti(L, LUA_REGISTRYINDEX, mRef);

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

            FSEntryHandle scriptHandle = mVfs->OpenHandle(path);
            
            std::string script, line;
            while (mVfs->ReadHandleLine(scriptHandle, &line)) {
                script += line + "\n";
            }

            mVfs->CloseHandle(scriptHandle);

            int top = lua_gettop(L);
            luaL_dostring(L, script.c_str());
            lua_settop(L, top); // Discard all values that the dostring() function could have pushed onto the stack
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

std::string Plugin::GetFileName() {
    return mFileName;
}

const Plugin::Properties Plugin::GetProperties() {
    if (Fail()) {
        Out("Plugin", "Plugin::GetProperties() called but plugin \"{}\" failed to initialize!", mFileName);
        return {};
    }
    lua_State *L = mCore->GetLuaState()->Get();
    PushLuaTable();

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

void Plugin::PushLuaTable() {
    if (Fail())
        return;
    lua_rawgeti(mCore->GetLuaState()->Get(), LUA_REGISTRYINDEX, mRef);
}