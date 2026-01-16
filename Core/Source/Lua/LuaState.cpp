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
// File: LuaState.cpp
// Started by: Hattozo
// Started on: 12/3/2025
// Description:
#include <NoobWarrior/Lua/LuaState.h>
#include <NoobWarrior/Lua/PluginEnv.h>
#include <NoobWarrior/Lua/LuaHypertextPreprocessor.h>
#include <NoobWarrior/Lua/VfsBinding.h>
#include <NoobWarrior/Lua/HttpServerBinding.h>
#include <NoobWarrior/Log.h>

#include "files/global_env_metatable.lua.inc.cpp"
#include "files/rawget_path.lua.inc.cpp"
#include "files/serpent.lua.inc.cpp"
#include "files/json.lua.inc.cpp"

using namespace NoobWarrior;

static int printBS(lua_State *L) {
    const char *str = luaL_checkstring(L, 1);
    Out("Lua", str);
    return 0;
}

LuaState::LuaState(Core* core) :
    L(nullptr),
    mCore(core),
    mLhp(this),
    mPluginEnv(this),
    mVfsBinding(this),
    mHttpServerBinding(this)
{}

int LuaState::Open() {
    L = luaL_newstate();

    luaL_openlibs(L);

    // Add reference to core in lua registry so that we can retrieve it for any bindings that require its existence
    lua_pushlightuserdata(L, mCore);
    lua_setfield(L, LUA_REGISTRYINDEX, "core");

    lua_pushcfunction(L, printBS);
    lua_setglobal(L, "print");

    // lua_pushcfunction(mLuaState, printBS);
    // lua_setglobal(mLuaState, "error");

    // Run lua code to define some functions without having to hassle with Lua C API
    luaL_dostring(L, rawget_path_lua);
    luaL_dostring(L, global_env_metatable_lua);

#define LOADLIBRARY(strVar, name) \
    int strVar##_exec_res = luaL_dostring(L, strVar); \
    if (!strVar##_exec_res && lua_istable(L, -1)) { \
        lua_setglobal(L, name); \
    } else lua_pop(L, 1);
    
    LOADLIBRARY(serpent_lua, "serpent")
    LOADLIBRARY(json_lua, "json")

#undef LOADLIBRARY

    mPluginEnv.Open();
    mVfsBinding.Open();
    mHttpServerBinding.Open();

    Out("Lua", "Initialized Lua");
    return 1;
}

void LuaState::Close() {
    mHttpServerBinding.Close();
    mVfsBinding.Close();
    mPluginEnv.Close();
    lua_close(L);
}

lua_State* LuaState::Get() {
    return L;
}