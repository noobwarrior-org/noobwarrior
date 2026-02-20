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
#include <NoobWarrior/Lua/LuaSignal.h>
#include <NoobWarrior/Lua/Bridge/PluginBridge.h>
#include <NoobWarrior/Lua/Lhp.h>
#include <NoobWarrior/Lua/Bridge/VfsBridge.h>
#include <NoobWarrior/Lua/Bridge/HttpServerBridge.h>
#include <NoobWarrior/Log.h>
#include <NoobWarrior/HttpServer/Base/HttpServer.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/NoobWarrior.h>

#include "files/global_env_metatable.lua.inc.cpp"
#include "files/rawget_path.lua.inc.cpp"
#include "files/serpent.lua.inc.cpp"
#include "files/json.lua.inc.cpp"

using namespace NoobWarrior;

static int printBS(lua_State *L) {
    int nargs = lua_gettop(L); 

    std::string msg;
    for (int i = 1; i <= nargs; i++) {
        const char *str = lua_tolstring(L, i, NULL);
        msg += str;
        lua_pop(L, 1);
    }
    Out("Lua", msg);
    return 0;
}

static int Listener(lua_State *L) {

    return 0;
}

LuaState::LuaState(Core* core) :
    L(nullptr),
    mCore(core),
    mLhp(this),
    mLuaSignalBridge(this),
    mPluginBridge(this),
    mLhpBridge(this),
    mVfsBridge(this),
    mHttpServerBridge(this),
    mServerEmulatorBridge(this)
{}

int LuaState::Open() {
    L = luaL_newstate();

    luaL_openlibs(L);

    // Run lua code to define some functions without having to hassle with Lua C API
    luaL_dostring(L, rawget_path_lua);
    luaL_dostring(L, global_env_metatable_lua);

    // Add reference to core in lua registry so that we can retrieve it for any bindings that require its existence
    lua_pushlightuserdata(L, mCore);
    lua_setfield(L, LUA_REGISTRYINDEX, "core");

    lua_pushcfunction(L, printBS);
    lua_setglobal(L, "print");

    // lua_pushcfunction(mLuaState, printBS);
    // lua_setglobal(mLuaState, "error");

#define LOADLIBRARY(strVar, name) \
    int strVar##_exec_res = luaL_dostring(L, strVar); \
    if (!strVar##_exec_res && lua_istable(L, -1)) { \
        lua_setglobal(L, name); \
    } else lua_pop(L, 1);
    
    LOADLIBRARY(serpent_lua, "serpent")
    LOADLIBRARY(json_lua, "json")

#undef LOADLIBRARY

    mLhpBridge.Open();
    mLuaSignalBridge.Open();
    mPluginBridge.Open();
    mVfsBridge.Open();
    mHttpServerBridge.Open();
    mServerEmulatorBridge.Open();

    Out("Lua", "Initialized Lua");
    return 1;
}

void LuaState::Close() {
    // Closing the bridges seen in LuaState::Open() is not required since Lua will just close it for us by using this function.
    lua_close(L);
    L = nullptr;
}

bool LuaState::Opened() {
    return L != nullptr;
}

lua_State* LuaState::Get() {
    return L;
}

Lhp *LuaState::GetLhp() {
    return &mLhp;
}

Core *LuaState::GetCore() {
    return mCore;
}
