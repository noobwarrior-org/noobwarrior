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
// File: LuaHypertextPreprocessor.cpp
// Started by: Hattozo
// Started on: 1/9/2026
// Description: LHP (Lua Hypertext Preprocessor)
// It's like PHP but for Lua
#include <NoobWarrior/Lua/LuaHypertextPreprocessor.h>
#include <NoobWarrior/Lua/LuaState.h>
#include <NoobWarrior/NoobWarrior.h>

#include <lua.hpp>

#include "files/lhp_env_metatable.lua.inc.cpp"

#define OPENING_TAG "<?lua"
#define CLOSING_TAG "?>"

using namespace NoobWarrior;

LuaHypertextPreprocessor::LuaHypertextPreprocessor(LuaState* lua) : mLua(lua) {

}

LuaHypertextPreprocessor::RenderResponse LuaHypertextPreprocessor::Render(const std::string &input, std::string *output) {
    bool lua_mode = false;
    std::string lua_block;
    for (int i = 0; i < input.size(); i++) {
        if (input.substr(i, NOOBWARRIOR_ARRAY_SIZE(OPENING_TAG) - 1) == OPENING_TAG) {
            // Switch to Lua mode, skip cursor to the first letter after the tag and restart
            lua_mode = true;
            lua_block = ""; // new block so reset string
            i += NOOBWARRIOR_ARRAY_SIZE(OPENING_TAG);
            continue;
        }

        if (input.substr(i, NOOBWARRIOR_ARRAY_SIZE(CLOSING_TAG) - 1) == CLOSING_TAG) {
            // end of block indicated by closing tag, turn off lua mode and execute code in block
            lua_mode = false;
            i += NOOBWARRIOR_ARRAY_SIZE(CLOSING_TAG);

            if (!lua_block.empty()) {
                lua_State *L = mLua->Get();
                int res = luaL_dostring(L, lua_block.c_str());
                if (res != LUA_OK) {
                    *output += lua_tostring(L, -1);
                    lua_pop(L, 1);
                }
            }
            continue;
        }

        if (!lua_mode)
            *output += input.at(i);
        else
            lua_block += input.at(i);
    }
    return RenderResponse::Success;
}