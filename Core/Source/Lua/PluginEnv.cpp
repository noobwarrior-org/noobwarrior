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
// File: PluginEnv.cpp
// Started by: Hattozo
// Started on: 1/10/2026
// Description:
#include <NoobWarrior/Lua/PluginEnv.h>
#include <NoobWarrior/Plugin.h>
#include <NoobWarrior/Log.h>

#include "files/plugin_env_metatable.lua.inc.cpp"

using namespace NoobWarrior;

PluginEnv::PluginEnv(LuaState* lua) : mLua(lua), mRef(0) {}

void PluginEnv::Open() {
    lua_State *L = mLua->Get();

    // Environment
    lua_newtable(L);
    mRef = luaL_ref(L, LUA_REGISTRYINDEX);

    Push();

    // Metatable
    int res = luaL_dostring(L, plugin_env_metatable_lua);
    if (res != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        Out("Lua", "Failed to create plugin environment: {}", err);
        lua_pop(L, 1);
        return;
    }
    lua_setmetatable(L, -2);
}

void PluginEnv::Close() {
    if (mRef == 0)
        return;
    lua_State *L = mLua->Get();
    luaL_unref(L, LUA_REGISTRYINDEX, mRef);
}

void PluginEnv::Push() {
    lua_State *L = mLua->Get();
    lua_rawgeti(L, LUA_REGISTRYINDEX, mRef);
}