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

Lhp::RenderResponse Lhp::Render(sol::environment env, const std::string &input, std::string *output, Url path, bool isRecursive) {
    bool luaMode = false;
    std::string textBuffer;
    std::string luaBuffer;
    std::vector<std::string> textBlocks;

    // very ugly code
    for (int i = 0; i < static_cast<int>(input.size()); i++) {
        if (input.substr(i, std::size(OPENING_TAG) - 1).compare(OPENING_TAG) == 0) {
            luaMode = true;
            i += std::size(OPENING_TAG) - 2;

            if (!textBuffer.empty()) {
                // luaBuffer += std::format("__tb[{}]();\n", textBlocks.size());
                luaBuffer += std::format("echo([=====[{}]=====]);\n", textBuffer);
                textBlocks.push_back(textBuffer);
                textBuffer.clear();
            }
            continue;
        }

        if (input.substr(i, std::size(CLOSING_TAG) - 1).compare(CLOSING_TAG) == 0) {
            if (!luaMode)
                return RenderResponse::SyntaxError;
            luaBuffer += '\n';
            luaMode = false;
            i += std::size(CLOSING_TAG) - 2;
            continue;
        }

        // Genuinely dogshit code that prevents string escaping. Never allow me to write again
        if (
            input.substr(i, 7).compare("[=====[") == 0 ||
            input.substr(i, 7).compare("]=====]") == 0
        )
        {
            Out("Lhp", "Continued");
            continue;
        }

        (!luaMode ? textBuffer : luaBuffer) += input.at(i);
    }
    if (!textBuffer.empty()) {
        // luaBuffer += std::format("__tb[{}]();\n", textBlocks.size());
        luaBuffer += std::format("echo([=====[{}]=====]);\n", textBuffer);
        textBlocks.push_back(textBuffer);
    }

    sol::table tb = mLua->create_table();
    for (int i = 0; i < static_cast<int>(textBlocks.size()); i++) {
        std::string block = textBlocks[i];
        tb[i] = [output, block]() {
            *output += block;
        };
    }

    sol::environment lhpEnv = isRecursive
        ? sol::environment(*mLua, sol::create, env) // creates an entirely new isolated environment
        : env; // or, reuse the passed environment directly. needed if we are calling this recursively or else all prior variables are lost

    if (isRecursive) {
        lhpEnv["__tb"] = tb;

        lhpEnv["echo"] = [output](std::string msg) -> void {
            *output += msg;
        };

        lhpEnv["include"] = [this, lhpEnv, output, path](sol::this_state state, std::string fileLocation) -> void {
            UrlContext ctx = {
                .Cwd = path.GetDirectory(),
                .DefaultProtocolType = path.GetProtocol(),
                .DefaultHostName = path.GetHostName()
            };

            lua_State* L = state;
            Url includeUrl(fileLocation, ctx);

            std::string includeOutput;
            RenderResponse res = Render(lhpEnv, includeUrl, &includeOutput, false);
            if (res != RenderResponse::Success) {
                luaL_error(L, "include() failed to render '%s'", fileLocation.c_str());
                return;
            }
            *output += includeOutput;
        };
    }

    sol::load_result bytecode = mLua->load(luaBuffer);
    if (!bytecode.valid()) {
        sol::error err = bytecode;
        Out("Lhp", "(Compile Failure) {}", err.what());
        return RenderResponse::LuaError;
    }

    sol::protected_function func = bytecode.get<sol::protected_function>();
    lhpEnv.set_on(func);
    sol::protected_function_result res = func();
    if (!res.valid()) {
        sol::error err = res;
        Out("Lhp", "(Render Failure) {}", err.what());
        return RenderResponse::LuaError;
    }
    return RenderResponse::Success;
}

Lhp::RenderResponse Lhp::Render(sol::environment env, const Url &url, std::string *output, bool isRecursive) {
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
    return Render(env, src, output, url, isRecursive);
}