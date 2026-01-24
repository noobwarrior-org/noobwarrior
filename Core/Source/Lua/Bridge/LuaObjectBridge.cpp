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
// File: LuaObjectBridge.cpp
// Started by: Hattozo
// Started on: 1/14/2026
// Description:
#include <NoobWarrior/Lua/Bridge/LuaObjectBridge.h>
#include <NoobWarrior/Lua/LuaState.h>

using namespace NoobWarrior;

static int gc(lua_State *L) {
    const char* mtName = luaL_checkstring(L, lua_upvalueindex(1));
    luaL_checkudata(L, 1, mtName);

    return 0;
}

static int index(lua_State *L) {
    const char* mtName = luaL_checkstring(L, lua_upvalueindex(1));
    luaL_checkudata(L, 1, mtName);

    return 0;
}

LuaObjectBridge::LuaObjectBridge(LuaState* lua, const std::string &mtName) :
    mLua(lua),
    mMtName(mtName)
{}

void LuaObjectBridge::Open() {
    lua_State *L = mLua->Get();

    luaL_newmetatable(L, mMtName.c_str());
    for (LuaRegEntry &entry : GetObjectMetaFuncs()) {
        lua_pushstring(L, mMtName.c_str());
        lua_pushcclosure(L, entry.second, 1);
        lua_setfield(L, -2, entry.first.c_str());
    }
    lua_pop(L, 1); // Since we don't need it for now

    LuaReg staticFuncs = GetStaticFuncs();
    if (staticFuncs.empty())
        return;

    lua_newtable(L);
    
    for (LuaRegEntry &entry : staticFuncs) {
        lua_pushcfunction(L, entry.second);
        lua_setfield(L, -2, entry.first.c_str());
    }

    lua_setglobal(L, mMtName.c_str());
}

void LuaObjectBridge::Close() {
    lua_State *L = mLua->Get();
    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, mMtName.c_str());
}

LuaReg LuaObjectBridge::GetStaticFuncs() {
    return {};
}

LuaReg LuaObjectBridge::GetObjectMetaFuncs() {
    return {
        {"__gc", gc},
        {"__index", index}
    };
}

LuaReg LuaObjectBridge::GetObjectFuncs() {
    return {};
}

LuaProperties LuaObjectBridge::GetObjectProps() {
    return {};
}
