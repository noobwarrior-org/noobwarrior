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
// File: PluginBridge.cpp
// Started by: Hattozo
// Started on: 1/17/2026
// Description:
#include <NoobWarrior/Lua/Bridge/PluginBridge.h>

using namespace NoobWarrior;

static int gc(lua_State *L) {
    PluginWrapper* p = (PluginWrapper*)luaL_checkudata(L, 1, "Plugin");
    if (p != nullptr) {
        p->~PluginWrapper();
    }
    return 0;
}

PluginBridge::PluginBridge(LuaState* lua) : LuaObjectBridge(lua, "Plugin") {

}

LuaReg PluginBridge::GetStaticFuncs() {
    return {}; // No constructors please.
}

LuaReg PluginBridge::GetObjectMetaFuncs() {
    return {
        {"__gc", gc},
        {"__index", gc}
    };
}

LuaReg PluginBridge::GetObjectFuncs() {
    return {
        {"GetIdentifier", gc},
        {"GetTitle", gc}
    };
}

PluginWrapper::PluginWrapper(Plugin* realPlugin) : mPlugin(realPlugin) {

}