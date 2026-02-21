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
// Description: An object representing a Lua script.
// This class primarily exists as a sort of "meta object" that contains their state.
// Basically imagine the "Script" instance in Roblox Studio
#include "NoobWarrior/FileSystem/VirtualFileSystem.h"
#include <NoobWarrior/Lua/LuaScript.h>
#include <NoobWarrior/Lua/LuaState.h>
#include <NoobWarrior/Log.h>
#include <sol/load_result.hpp>
#include <sol/protected_function_result.hpp>
#include <sol/types.hpp>
#include <sol/variadic_args.hpp>

using namespace NoobWarrior;

// btw: you can pass sol::environment by copy and it will still reference the same Lua environment
LuaScript::LuaScript(LuaState* lua, sol::environment env, const Url &identifier) :
    mLua(lua),
    mUrl(identifier),
    mFailReason(FailReason::Unknown),
    mBaseEnv(std::move(env))
{
    if (!lua->Opened()) {
        Out("LuaScript", "[{}] (Load Failure) {}", mUrl.Resolve(), "Tried compiling script but Lua subsystem is not open!");
        mFailReason = FailReason::LuaNotOpen;
        return;
    }
    Core* core = lua->GetCore();

    VirtualFileSystem* vfs = nullptr;
    FSEntryHandle scriptHandle;

    VirtualFileSystem::Response fileRes = mUrl.OpenHandle(core, &vfs, &scriptHandle);
    if (vfs == nullptr) {
        Out("LuaScript", "[{}] (Load Failure) {}", mUrl.Resolve(), "Failed to retrieve the plugin filesystem.");
        mFailReason = FailReason::UrlFailed;
        return;
    }
    if (scriptHandle == 0) {
        Out("LuaScript", "[{}] (Load Failure) {}", mUrl.Resolve(), "The file handle failed to open.");
        mFailReason = FailReason::UrlFailed;
        return;
    }

    std::string src, line;
    while (vfs->ReadHandleLine(scriptHandle, &line)) {
        src += line + "\n";
    }

    vfs->CloseHandle(scriptHandle);

    mSource = src;

    mBytecode = mLua->load(src);
    if (!mBytecode.valid()) {
        sol::error err = mBytecode;
        Out("LuaScript", "[{}] (Compile Failure) {}", mUrl.Resolve(), err.what());
    }
    switch (mBytecode.status()) {
    case sol::load_status::ok: mFailReason = FailReason::None; break;
    case sol::load_status::syntax: mFailReason = FailReason::SyntaxError; return;
    default: mFailReason = FailReason::Unknown; return;
    }
}

LuaScript::LuaScript(LuaState* lua, sol::environment env, const std::string &src) :
    mLua(lua),
    mSource(src),
    mFailReason(FailReason::Unknown),
    mBaseEnv(std::move(env))
{
    if (!lua->Opened()) {
        Out("LuaScript", "[{}] (Load Failure) {}", mUrl.Resolve(), "Tried compiling script but Lua subsystem is not open!");
        mFailReason = FailReason::LuaNotOpen;
        return;
    }

    mBytecode = mLua->load(src);
    if (!mBytecode.valid()) {
        sol::error err = mBytecode;
        Out("LuaScript", "[{}] (Compile Failure) {}", mUrl.Resolve(), err.what());
    }
    switch (mBytecode.status()) {
    case sol::load_status::ok: mFailReason = FailReason::None; break;
    case sol::load_status::syntax: mFailReason = FailReason::SyntaxError; return;
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
    if (Fail()) return ExecResponse::InitFailed;

    sol::environment sandbox = sol::environment(*mLua, sol::create, mBaseEnv);
    sandbox["script"] = this;
    sandbox["print"] = [this](sol::variadic_args args) {
        std::string msg;
        for (auto arg : args) {
            if (!msg.empty()) msg += " ";
            sol::protected_function_result res = (*mLua)["tostring"].call(arg);
            if (res.valid() && res.get_type() == sol::type::string)
                msg += res.get<std::string>();
        }
        if (!mUrl.IsBlank())
            Out("LuaScript", "[{}] {}", mUrl.Resolve(), msg);
        else
            Out("LuaScript", msg);
    };

    sol::protected_function func = mBytecode.get<sol::protected_function>();
    sandbox.set_on(func);
    sol::protected_function_result res = func();
    if (!res.valid()) {
        sol::error err = res;
        Out("LuaScript", "[{}] (Execution Failure) {}", mUrl.Resolve(), err.what());
    }
    switch (res.status()) {
    case sol::call_status::ok: return ExecResponse::Ok;
    default: return ExecResponse::Failed;
    }
}

Url& LuaScript::GetUrl() {
    return mUrl;
}

std::string LuaScript::GetSource() {
    return mSource;
}