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
// File: BaseConfig.h
// Started by: Hattozo
// Started on: 6/23/2025
// Description:
#pragma once
#include <NoobWarrior/Log.h>

#include <NoobWarrior/Lua/LuaState.h>

#include <sol/sol.hpp>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <map>
#include <any>
#include <vector>
#include <format>
#include <mutex>

#define NOOBWARRIOR_CONFIG_DESERIALIZE_ENUM(propName, key, enumeration) \
    { \
        lua_getfield(mLuaState, -1, key); \
        if (!lua_isnil(mLuaState, -1) && lua_type(mLuaState, -1) == LUA_TNUMBER) \
            propName = static_cast<enumeration>(lua_tonumber(mLuaState, -1)); \
        lua_pop(mLuaState, 1); \
    }

#define NOOBWARRIOR_CONFIG_DESERIALIZE_NUMBER(propName, key) \
    { \
        lua_getfield(mLuaState, -1, key); \
        if (!lua_isnil(mLuaState, -1) && lua_type(mLuaState, -1) == LUA_TNUMBER) \
            propName = lua_tonumber(mLuaState, -1); \
        lua_pop(mLuaState, 1); \
    }

#define NOOBWARRIOR_CONFIG_DESERIALIZE_STRING(propName, key) \
    { \
        lua_getfield(mLuaState, -1, key); \
        if (!lua_isnil(mLuaState, -1) && lua_type(mLuaState, -1) == LUA_TSTRING) \
            propName = std::string(lua_tostring(mLuaState, -1)); \
        lua_pop(mLuaState, 1); \
    }

namespace NoobWarrior {
enum class ConfigResponse {
    Failed,
    Success,
    CantReadFile,
    MemoryError,
    SyntaxError,
    ErrorDuringExecution,
    ReturningWrongType
};
class BaseConfig {
public:
    BaseConfig(std::string globalName, std::filesystem::path filePath, LuaState* lua);
    virtual ConfigResponse Open();
    ConfigResponse Close();
    std::string GetLuaError();

    void SetKeyComment(const char *key, const char *comment);

    template <typename T>
    void SetKeyValue(const std::string &key, T value) {
        if (key.empty()) {
            Out("BaseConfig", "Error setting value for key \"{}\": key is empty", key);
            return;
        }
        (*mLua)["__REG_BUF"] = value;
        sol::protected_function_result res = mLua->safe_script(std::format("{}.{} = __REG_BUF", mGlobalName, key));
        if (!res.valid()) {
            const sol::error err = res;
            Out("BaseConfig", "Error setting value for key \"{}\": \"{}\"", key, err.what());
        }
        (*mLua)["__REG_BUF"] = sol::lua_nil;
    }

    template <typename T>
    std::optional<T> GetKeyValue(const std::string &key) {
        if (key.empty()) {
            Out("BaseConfig", "Error getting value for key \"{}\": key is empty", key);
            return std::nullopt;
        }
        sol::protected_function_result res = mLua->safe_script(std::format("return {}.{}", mGlobalName, key), sol::script_pass_on_error);
        if (!res.valid()) {
            return std::nullopt;
        }
        sol::object obj = res.get<sol::object>();
        if (!obj.is<T>()) {
            return std::nullopt;
        }
        return obj.as<T>();
    }

    template <typename T>
    void SetKeyValueIfNotSet(const std::string &key, T value) {
        if (key.empty() || GetKeyValue<T>(key).has_value())
            return;
        SetKeyValue<T>(key, value);
    }
protected:
    std::string             mGlobalName;
    std::string             mLastError;
    std::filesystem::path   mFilePath;
    std::ostream*           mFileOutput;
    LuaState*               mLua;

    std::mutex AccessConfigMutex;
};
}