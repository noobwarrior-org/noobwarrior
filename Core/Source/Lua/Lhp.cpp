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
// File: Lhp.cpp
// Started by: Hattozo
// Started on: 1/9/2026
// Description: LHP (Lua Hypertext Preprocessor)
// It's like PHP but for Lua
#include <NoobWarrior/Lua/Lhp.h>
#include <NoobWarrior/Lua/LuaState.h>
#include <NoobWarrior/NoobWarrior.h>

#include <lua.hpp>

#include "files/lhp_env_metatable.lua.inc.cpp"

#define OPENING_TAG "<?lua"
#define CLOSING_TAG "?>"

using namespace NoobWarrior;

Lhp::Lhp(LuaState* lua) : mLua(lua) {

}

Lhp::RenderResponse Lhp::Render(const std::string &input, std::string *output) {
    bool lua_mode = false;
    bool has_processed_text_yet = false;
    std::string text_block;
    std::string lua_output;

    for (int i = 0; i < input.size(); i++) {
        if (input.substr(i, NOOBWARRIOR_ARRAY_SIZE(OPENING_TAG) - 1) == OPENING_TAG) {
            // Switch to Lua mode, skip cursor to the first letter after the tag, write down the bytes from the previous text block, and restart
            lua_mode = true;
            has_processed_text_yet = true;
            i += NOOBWARRIOR_ARRAY_SIZE(OPENING_TAG);

            if (!text_block.empty()) {
                lua_output += std::format("echo([[{}]]);", text_block);
            }
            
            continue;
        }

        if (input.substr(i, NOOBWARRIOR_ARRAY_SIZE(CLOSING_TAG) - 1) == CLOSING_TAG) {
            if (!lua_mode)
                return RenderResponse::SyntaxError;

            // end of block indicated by closing tag, turn off lua mode and execute code in block
            lua_mode = false;
            i += NOOBWARRIOR_ARRAY_SIZE(CLOSING_TAG);

            /*
            if (!lua_block.empty()) {
                lua_State *L = mLua->Get();
                int res = luaL_dostring(L, lua_block.c_str());
                if (res != LUA_OK) {
                    *output += lua_tostring(L, -1);
                    lua_pop(L, 1);
                }
            }
            */
            continue;
        }

        (!lua_mode ? text_block : lua_output) += input.at(i);

        if (!has_processed_text_yet) {
            lua_output += std::format("echo([[{}]]);", text_block);
        }

        Out("Lhp", lua_output);
    }
    return RenderResponse::Success;
}

Lhp::RenderResponse Lhp::Render(const Url &url, std::string *output) {
    Core* core = mLua->GetCore();

    VirtualFileSystem* vfs = nullptr;
    FSEntryHandle sourceHandle;

    VirtualFileSystem::Response fileRes = url.OpenHandle(core, &vfs, &sourceHandle);
    if (vfs == nullptr || sourceHandle == 0) {
        return RenderResponse::UrlFailed;
    }

    std::string src, line;
    while (vfs->ReadHandleLine(sourceHandle, &line)) {
        src += line + "\n";
    }

    vfs->CloseHandle(sourceHandle);

    return Render(src, output);
}