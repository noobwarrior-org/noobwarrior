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

Lhp::RenderResponse Lhp::Render(sol::environment env, const std::string &input, std::string *output) {
    bool luaMode = false;
    std::string textBuffer;
    std::string luaBuffer;
    
    for (int i = 0; i < input.size(); i++) {
        if (input.substr(i, NOOBWARRIOR_ARRAY_SIZE(OPENING_TAG) - 1).compare(OPENING_TAG) == 0) {
            // Switch to Lua mode, skip cursor to the first letter after the tag, write down the bytes from the previous text block, and restart
            luaMode = true;
            i += NOOBWARRIOR_ARRAY_SIZE(OPENING_TAG) - 2;

            if (!textBuffer.empty()) {
                luaBuffer += std::format("echo([[{}]]);", textBuffer);
                textBuffer.clear();
            }
            
            continue;
        }

        if (input.substr(i, NOOBWARRIOR_ARRAY_SIZE(CLOSING_TAG) - 1).compare(CLOSING_TAG) == 0) {
            if (!luaMode)
                return RenderResponse::SyntaxError;

            // end of block indicated by closing tag, turn off lua mode and execute code in block
            luaMode = false;
            i += NOOBWARRIOR_ARRAY_SIZE(CLOSING_TAG) - 2;
            continue;
        }

        if (
            input.size() > i + 1
            &&
            ((input.at(i) == '[' && input.at(i + 1) == '[')
            || (input.at(i) == ']' && input.at(i + 1) == ']'))
        )
        {
            continue; // prevent string escaping
        }

        (!luaMode ? textBuffer : luaBuffer) += input.at(i);
    }
    if (!textBuffer.empty()) {
        luaBuffer += std::format("echo([[{}]]);", textBuffer);
    }

    sol::environment lhpEnv(env);
    lhpEnv["echo"] = [output](std::string msg) -> void {
        *output += msg;
    };

    sol::protected_function_result res = mLua->safe_script(luaBuffer, lhpEnv);

    return RenderResponse::Success;
}

Lhp::RenderResponse Lhp::Render(sol::environment env, const Url &url, std::string *output) {
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

    return Render(env, src, output);
}