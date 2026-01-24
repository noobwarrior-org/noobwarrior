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
// File: LuaObjectBridge.h
// Started by: Hattozo
// Started on: 1/14/2026
// Description:
#pragma once
#include <string>
#include <vector>
#include <utility>
#include <lua.hpp>

namespace NoobWarrior {
typedef std::pair<std::string, lua_CFunction> LuaRegEntry;
typedef std::vector<LuaRegEntry> LuaReg;

typedef std::tuple<std::string, lua_CFunction, lua_CFunction> LuaProperty;
typedef std::vector<LuaProperty> LuaProperties;

class LuaState;
class LuaObjectBridge {
public:
    LuaObjectBridge(LuaState* lua, const std::string &mtName);
    virtual void Open();
    virtual void Close();
    virtual LuaReg GetStaticFuncs();
    virtual LuaReg GetObjectMetaFuncs();
    virtual LuaReg GetObjectFuncs();
    virtual LuaProperties GetObjectProps();
protected:
    LuaState* mLua;
    std::string mMtName;
};
}