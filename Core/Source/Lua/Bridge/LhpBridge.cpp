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
// File: LhpBridge.cpp
// Started by: Hattozo
// Started on: 1/15/2026
// Description:
#include <NoobWarrior/Lua/Bridge/LhpBridge.h>
#include <NoobWarrior/Lua/LuaState.h>
#include <NoobWarrior/Lua/LuaHypertextPreprocessor.h>
#include <NoobWarrior/Url.h>
#include <NoobWarrior/NoobWarrior.h>

using namespace NoobWarrior;
using Lhp = LuaHypertextPreprocessor; // maybe I should shorten the class name one day

static int Render(lua_State *L) {
    lua_getfenv(L, 1);
    lua_getfield(L, -1, "script");
    if (lua_isnil(L, -1)) {
        lua_getfield(L, -1, "GetUrl");
        lua_pcall(L, 0, 1, 0);
    }
    const char* filePath = luaL_checkstring(L, 1);

    Url source(filePath, {
        .Cwd = "",
        .DefaultProtocolType = ProtocolType::Plugin,
        .DefaultHostName = ""
    });

    std::string renderedOutput;

    lua_getfield(L, LUA_REGISTRYINDEX, "core");
    auto core = (Core*)lua_topointer(L, -1);
    lua_pop(L, 1);

    Lhp::RenderResponse res = core->GetLuaState()->GetLhp()->Render(source, &renderedOutput);
    if (res != Lhp::RenderResponse::Success) {
        return luaL_error(L, "failed to render file \"%s\"", filePath);
    }

    lua_pushstring(L, renderedOutput.c_str());
    return 1;
}

static const luaL_Reg Funcs[] {
    {"Render", Render},
    {NULL, NULL}
};

LhpBridge::LhpBridge(LuaState* lua) : mLua(lua) {}

void LhpBridge::Open() {
    lua_State *L = mLua->Get();

    luaL_newlib(L, Funcs);
    lua_setglobal(L, "lhp");
}

void LhpBridge::Close() {
    lua_State *L = mLua->Get();
    lua_pushnil(L);
    lua_setglobal(L, "lhp");
}