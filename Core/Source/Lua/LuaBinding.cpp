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
// File: LuaBinding.cpp
// Started by: Hattozo
// Started on: 1/14/2026
// Description:
#include <NoobWarrior/Lua/LuaBinding.h>
#include <NoobWarrior/Lua/LuaState.h>

using namespace NoobWarrior;

LuaBinding::LuaBinding(LuaState* lua, const std::string &mtName) :
    mLua(lua),
    mMtName(mtName)
{}

void LuaBinding::Open() {
    lua_State *L = mLua->Get();
    luaL_newmetatable(L, mMtName.c_str());
    lua_pop(L, 1); // Since we don't need it for now

    lua_newtable(L);
    
    for (LuaRegEntry entry : GetLibFuncs()) {
        lua_pushcfunction(L, entry.second);
        lua_setfield(L, -2, entry.first.c_str());
    }
    
    lua_setglobal(L, mMtName.c_str());
}

void LuaBinding::Close() {
    lua_State *L = mLua->Get();
    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, mMtName.c_str());
}
