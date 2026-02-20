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
#include <sol/sol.hpp>

#include <NoobWarrior/Lua/LuaScript.h>
#include <NoobWarrior/Lua/Lhp.h>
#include <NoobWarrior/Lua/Bridge/LuaSignalBridge.h>
#include <NoobWarrior/Lua/Bridge/PluginBridge.h>
#include <NoobWarrior/Lua/Bridge/LhpBridge.h>
#include <NoobWarrior/Lua/Bridge/VfsBridge.h>
#include <NoobWarrior/Lua/Bridge/HttpServerBridge.h>
#include <NoobWarrior/Lua/Bridge/ServerEmulatorBridge.h>

namespace NoobWarrior {
class Core;
enum class LuaContext {
    NoPlugin,
    InstallPlugin,
    UserPlugin,
};
class LuaState : public sol::state {
public:
    LuaState(Core* core);
    int Open();
    bool Opened();

    lua_State* Get();
    Lhp *GetLhp();
    Core *GetCore();
private:
    Core* mCore;
    Lhp mLhp;
    LuaSignalBridge mLuaSignalBridge;
    PluginBridge mPluginBridge;
    LhpBridge mLhpBridge;
    VfsBridge mVfsBridge;
    HttpServerBridge mHttpServerBridge;
    ServerEmulatorBridge mServerEmulatorBridge;
};
}