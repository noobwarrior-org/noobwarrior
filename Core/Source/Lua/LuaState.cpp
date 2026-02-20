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
// Description: Uses sol
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
#include <sol/forward.hpp>

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
    mCore(core),
    mLhp(this),
    mLuaSignalBridge(this),
    mPluginBridge(this),
    mLhpBridge(this),
    mVfsBridge(this),
    mHttpServerBridge(this),
    mServerEmulatorBridge(this)
{
    open_libraries(
        sol::lib::base,
        sol::lib::package,
        sol::lib::coroutine,
        sol::lib::string,
        sol::lib::os,
        sol::lib::math,
        sol::lib::table,
        sol::lib::debug,
        sol::lib::bit32,
        sol::lib::io,
        sol::lib::ffi,
        sol::lib::jit
    );

    do_string(rawget_path_lua);
    do_string(global_env_metatable_lua);

    set("core", sol::make_light(mCore));
    set("print", printBS);
}

int LuaState::Open() {
    // lua_pushcfunction(mLuaState, printBS);
    // lua_setglobal(mLuaState, "error");

#define LOADLIBRARY(strVar, name) \
    sol::protected_function_result strVar##_res = do_string(strVar); \
    if (strVar##_res.valid() &&strVar##_res.get_type() == sol::type::table) { \
        set(name, strVar##_res.get<sol::table>()); \
    }
    
    LOADLIBRARY(serpent_lua, "serpent")
    LOADLIBRARY(json_lua, "json")

    // sol::protected_function_result res = do_string(serpent_lua);
    // if (res.valid() &&res.get_type() == sol::type::table) {
    //     set("serpent", res.get<sol::table>());
    // }
#undef LOADLIBRARY

    // mLhpBridge.Open();
    // mLuaSignalBridge.Open();
    // mPluginBridge.Open();
    // mVfsBridge.Open();
    // mHttpServerBridge.Open();
    // mServerEmulatorBridge.Open();

    Out("Lua", "Initialized Lua");
    return 1;
}

bool LuaState::Opened() {
    return true;
}

lua_State* LuaState::Get() {
    return lua_state();
}

Lhp *LuaState::GetLhp() {
    return &mLhp;
}

Core *LuaState::GetCore() {
    return mCore;
}
