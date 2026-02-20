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
#include "NoobWarrior/Plugin.h"
#include <NoobWarrior/Lua/Bridge/ServerEmulatorBridge.h>
#include <NoobWarrior/Lua/LuaState.h>
#include <NoobWarrior/Log.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/NoobWarrior.h>

using namespace NoobWarrior;

static int New(lua_State *L) {
    lua_pushnil(L);
    return 1;
}

static int BanUserId(lua_State *L) {
    lua_pushnil(L);
    return 1;
}

static const luaL_Reg MetaFuncs[] {
    {NULL, NULL}
};

static const luaL_Reg ObjectFuncs[] {
    {"new", New},
    {"BanUserId", BanUserId},
    {NULL, NULL}
};

ServerEmulatorBridge::ServerEmulatorBridge(LuaState* lua) : mLua(lua) {}

void ServerEmulatorBridge::Open() {
    lua_State *L = mLua->Get();

    // fuck this shit
    luaL_newmetatable(L, "ServerEmulator");
    luaL_setfuncs(L, MetaFuncs, 0);

    lua_newtable(L);
    luaL_setfuncs(L, ObjectFuncs, 0);

    lua_newtable(L); // our nested metatable for the server emulator's __index metamethod
    luaL_getmetatable(L, "HttpServer");
    lua_setfield(L, -2, "__index");
    lua_setmetatable(L, -2);

    lua_setfield(L, -2, "__index");

    lua_pop(L, 1);

    // Let lua memory manage a single pointer instead of the actual data, cuz C++ memory manages the real shit
    ServerEmulator** ptr = static_cast<ServerEmulator**>(lua_newuserdata(L, sizeof(ServerEmulator*)));
    *ptr = mLua->GetCore()->GetServerEmulator();
    luaL_setmetatable(L, "ServerEmulator");
    lua_setglobal(L, "emu");
}
