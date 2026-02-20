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
// File: LuaSignalBridge.cpp
// Started by: Hattozo
// Started on: 2/19/2026
// Description:
#include <NoobWarrior/Lua/Bridge/LuaSignalBridge.h>
#include <NoobWarrior/Lua/LuaSignal.h>

using namespace NoobWarrior;

static int Connect(lua_State *L) {
    lua_pushstring(L, "poop");
    return 1;
}

static int New(lua_State *L) {
    LuaSignal *signal = (LuaSignal*)lua_newuserdata(L, sizeof(LuaSignal));
    new(signal) LuaSignal();

    luaL_setmetatable(L, "Signal");

    return 1;
}

LuaSignalBridge::LuaSignalBridge(LuaState* lua) : LuaObjectBridge(lua, "Signal") {

}

LuaReg LuaSignalBridge::GetStaticFuncs() {
    return {
        {"new", New}
    };
}

LuaReg LuaSignalBridge::GetObjectFuncs() {
    return {
        {"Connect", Connect}
    };
}
