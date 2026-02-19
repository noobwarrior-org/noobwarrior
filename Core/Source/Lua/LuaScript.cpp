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
// File: LuaScript.cpp
// Started by: Hattozo
// Started on: 1/18/2026
// Description:
#include "NoobWarrior/FileSystem/VirtualFileSystem.h"
#include <NoobWarrior/Lua/LuaScript.h>
#include <NoobWarrior/Lua/LuaState.h>
#include <NoobWarrior/Log.h>

using namespace NoobWarrior;

LuaScript::LuaScript(LuaState* lua, const Url &identifier) : mLua(lua), mUrl(identifier), mFailReason(FailReason::Unknown) {
    if (!lua->Opened()) {
        Out("LuaScript", "[{}] (Load Failure) {}", mUrl.Resolve(), "Tried compiling script but Lua subsystem is not open!");
        mFailReason = FailReason::LuaNotOpen;
        return;
    }
    lua_State *L = lua->Get();
    Core* core = lua->GetCore();

    VirtualFileSystem* vfs = nullptr;
    FSEntryHandle scriptHandle;

    VirtualFileSystem::Response fileRes = mUrl.OpenHandle(core, &vfs, &scriptHandle);
    if (vfs == nullptr) {
        Out("Lua", "[{}] (Load Failure) {}", mUrl.Resolve(), "Failed to retrieve the plugin filesystem.");
        mFailReason = FailReason::UrlFailed;
        return;
    }
    if (scriptHandle == 0) {
        Out("Lua", "[{}] (Load Failure) {}", mUrl.Resolve(), "The file handle failed to open.");
        mFailReason = FailReason::UrlFailed;
        return;
    }

    std::string src, line;
    while (vfs->ReadHandleLine(scriptHandle, &line)) {
        src += line + "\n";
    }

    vfs->CloseHandle(scriptHandle);

    mSource = src;

    int res = luaL_loadstring(L, src.c_str());
    if (res != LUA_OK) {
        Out("Lua", "[{}] (Compile Failure) {}", mUrl.Resolve(), lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    switch (res) {
    case LUA_OK: mFailReason = FailReason::None; break;
    case LUA_ERRSYNTAX: mFailReason = FailReason::SyntaxError; return;
    default: mFailReason = FailReason::Unknown; return;
    }
}

LuaScript::LuaScript(LuaState* lua, const std::string &src) : mLua(lua), mSource(src) {
    if (!lua->Opened()) {
        Out("LuaScript", "[{}] (Load Failure) {}", mUrl.Resolve(), "Tried compiling script but Lua subsystem is not open!");
        mFailReason = FailReason::LuaNotOpen;
        return;
    }
    lua_State *L = lua->Get();

    int res = luaL_loadstring(L, src.c_str());
    if (res != LUA_OK) {
        Out("Lua", "[{}] (Compile Failure) {}", mUrl.Resolve(), lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    switch (res) {
    case LUA_OK: mFailReason = FailReason::None; break;
    case LUA_ERRSYNTAX: mFailReason = FailReason::SyntaxError; return;
    default: mFailReason = FailReason::Unknown; return;
    }
}

bool LuaScript::Fail() {
    return mFailReason != FailReason::None;
}

LuaScript::FailReason LuaScript::GetFailReason() {
    return mFailReason;
}

LuaScript::ExecResponse LuaScript::Execute() {
    lua_State *L = mLua->Get();
    int res = lua_pcall(L, 0, 0, 0);
    if (res != LUA_OK) {
        Out("Lua", "[{}] (Execution Failure) {}", mUrl.Resolve(), lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    switch (res) {
    case LUA_OK: return ExecResponse::Ok;
    default: return ExecResponse::Failed;
    }
}

Url& LuaScript::GetUrl() {
    return mUrl;
}

std::string LuaScript::GetSource() {
    return mSource;
}