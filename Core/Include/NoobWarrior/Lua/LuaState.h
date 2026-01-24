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
// File: LuaState.h
// Started by: Hattozo
// Started on: 12/3/2025
// Description:
#pragma once
#include <lua.hpp>

#include <NoobWarrior/Lua/LuaScript.h>
#include <NoobWarrior/Lua/LuaHypertextPreprocessor.h>
#include <NoobWarrior/Lua/Bridge/PluginBridge.h>
#include <NoobWarrior/Lua/Bridge/LhpBridge.h>
#include <NoobWarrior/Lua/Bridge/VfsBridge.h>
#include <NoobWarrior/Lua/Bridge/HttpServerBridge.h>

namespace NoobWarrior {
class Core;
enum class LuaContext {
    NoPlugin,
    InstallPlugin,
    UserPlugin,
};
class LuaState {
public:
    LuaState(Core* core);
    int Open();
    void Close();
    bool Opened();

    lua_State* Get();
    LuaHypertextPreprocessor *GetLhp();
    Core *GetCore();
private:
    lua_State *L;

    Core* mCore;
    LuaHypertextPreprocessor mLhp;
    PluginBridge mPluginBinding;
    LhpBridge mLhpBinding;
    PluginBridge mVfsBinding;
    HttpServerBridge mHttpServerBinding;
};
}