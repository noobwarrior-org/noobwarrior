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
// File: ServerEmulatorBridge.cpp
// Started by: Hattozo
// Started on: 2/19/2026
// Description:
#include <NoobWarrior/Lua/Bridge/ServerEmulatorBridge.h>
#include <NoobWarrior/Lua/LuaState.h>
#include <NoobWarrior/Log.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/NoobWarrior.h>

using namespace NoobWarrior;

static int ServerEmulator_index(lua_State *L) {
    luaL_getmetatable(L, "HttpServer");
    lua_getfield(L, -1, "__index");

    if (lua_isfunction(L, -1)) {
        lua_pushvalue(L, 1); // self
        lua_pushvalue(L, 2); // key
        lua_call(L, 2, 1);
        return 1;
    }
    if (lua_istable(L, -1)) {
        lua_pushvalue(L, 2); // key
        lua_gettable(L, -2);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

ServerEmulatorBridge::ServerEmulatorBridge(LuaState* lua) : LuaObjectBridge(lua, "ServerEmulator") {}

void ServerEmulatorBridge::Open() {
    LuaObjectBridge::Open();
    lua_State *L = mLua->Get();

    // Let lua memory manage a single pointer instead of the actual data, cuz C++ memory manages the real shit
    ServerEmulator** ptr = static_cast<ServerEmulator**>(lua_newuserdata(L, sizeof(ServerEmulator*)));
    *ptr = mLua->GetCore()->GetServerEmulator();
    luaL_setmetatable(L, "ServerEmulator");
    lua_setglobal(L, "emu");
}

LuaReg ServerEmulatorBridge::GetObjectMetaFuncs() {
    return {
        {"__index", ServerEmulator_index}
    };
}