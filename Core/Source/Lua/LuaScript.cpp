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
    mUrl.OpenHandle(core, &vfs, &scriptHandle);
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
}

LuaScript::~LuaScript() {
    mResult.reset();
    mBaseEnv.abandon();
}

bool LuaScript::Fail() {
    return mFailReason != FailReason::None;
}

LuaScript::FailReason LuaScript::GetFailReason() {
    return mFailReason;
}

sol::protected_function_result LuaScript::Execute() {
    if (Fail()) {
        Out("LuaScript", "[{}] (Execution Failure) The script failed to compile so it cannot execute.", mUrl.Resolve());
        sol::protected_function_result res = mLua->safe_script("return nil");
        return res;
    }

    sol::environment sandbox = sol::environment(*mLua, sol::create, mBaseEnv);
    sandbox["script"] = this;
    
    sandbox["print"] = [this](sol::this_state state, sol::variadic_args args) {
        sol::state_view lua(state);
        std::string msg;
        for (auto arg : args) {
            if (!msg.empty()) msg += " ";
            sol::protected_function_result res = lua["tostring"].call(arg);
            if (res.valid() && res.get_type() == sol::type::string) {
                msg += res.get<std::string>();
            } else {
                msg += "[unprintable]";
            }
        }
        if (!mUrl.IsBlank())
            Out("LuaScript", "[{}] {}", mUrl.Resolve(), msg);
        else
            Out("LuaScript", msg);
    };

    sandbox["require"] = [this](sol::this_state state, const std::string& urlStr) -> sol::object {
        lua_State* L = state;
        Url url = Url(urlStr);
        std::string resolvedUrl = url.Resolve();

        VirtualFileSystem* moduleVfs = url.GetVfs(mLua->GetCore());
        if (moduleVfs == nullptr) {
            // TODO: figure out a way to make this error less cryptic
            luaL_error(L, "require(): cannot require \"%s\" as vfs for url \"%s\" doesn't exist",
                resolvedUrl.c_str(), std::string(url.GetProtocolString() + "://" + url.GetHostName()).c_str());
            return sol::lua_nil;
        }

        if (moduleVfs->EntryExists(url.ResolveAsPath())) {
            luaL_error(L, "require(): url \"%s\" doesn't exist on disk", resolvedUrl.c_str());
            return sol::lua_nil;
        }

        LuaScript* cachedModule = mLua->RetrieveCachedModuleFromResolvedUrl(resolvedUrl);
        if (cachedModule != nullptr) {
            return cachedModule->GetLastResult();
        }

        if (mLua->IsScriptLoading(resolvedUrl)) {
            luaL_error(L, "require(): circular dependency detected: %s", resolvedUrl.c_str());
        }

        mLua->MarkScriptLoading(resolvedUrl);
        auto moduleScript = std::make_unique<LuaScript>(mLua, mLua->globals(), url);
        if (moduleScript->Fail()) {
            mLua->UnmarkScriptLoading(resolvedUrl);
            luaL_error(L, "require(): failed to load script: %s", resolvedUrl.c_str());
            return sol::lua_nil;
        }
        sol::protected_function_result res2 = moduleScript->Execute();
        if (!res2.valid()) {
            mLua->UnmarkScriptLoading(resolvedUrl);
            luaL_error(L, "require(): failed to execute script: %s", resolvedUrl.c_str());
            return sol::lua_nil;
        }
        mLua->CacheModule(resolvedUrl, std::move(moduleScript));
        mLua->UnmarkScriptLoading(resolvedUrl);

        if (!res2.valid()) {
            luaL_error(L, "require(): failed to run script");
            return sol::lua_nil;
        }
        return res2;
    };

    sol::load_result bytecode = mLua->load(mSource);
    if (!bytecode.valid()) {
        sol::error err = bytecode;
        sol::protected_function_result res = mLua->safe_script("return nil");
        return res;
    }

    auto func = bytecode.get<sol::protected_function>();
    sandbox.set_on(func);
    sol::protected_function_result res = func();
    if (!res.valid()) {
        sol::error err = res;
        Out("LuaScript", "[{}] (Execution Failure) {}", mUrl.Resolve(), err.what());
    } else {
        mResult = res.get<sol::object>();
    }
    return res;
}

sol::object LuaScript::GetLastResult() {
    if (mResult.has_value())
        return mResult.value();
    return sol::lua_nil;
}

Url& LuaScript::GetUrl() {
    return mUrl;
}

std::string LuaScript::GetSource() {
    return mSource;
}