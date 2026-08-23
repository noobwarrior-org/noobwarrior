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
// File: BaseRegistry.h
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

namespace NoobWarrior {
enum class RegistryResponse {
    Failed,
    Success,
    CantReadFile,
    MemoryError,
    SyntaxError,
    ErrorDuringExecution,
    ReturningWrongType
};
class BaseRegistry {
public:
    BaseRegistry(std::string globalName, std::filesystem::path filePath, LuaState* lua);
    virtual RegistryResponse Open();
    RegistryResponse Save();
    RegistryResponse Close();
    std::string GetLuaError();

    void SetKeyComment(const char *key, const char *comment);

    std::optional<sol::object> RawGetKeyObject(const std::string &key);
    bool HasKey(const std::string &key);

    template <typename T>
    void SetKeyValue(const std::string &key, T value) {
        if (key.empty()) {
            Out("BaseRegistry", "Error setting value for key \"{}\": key is empty", key);
            return;
        }
        if (key.starts_with('.')) {
            Out("BaseRegistry", "Error setting value for key \"{}\": key cannot start with a period", key);
            return;
        }
        if (key.ends_with('.')) {
            Out("BaseRegistry", "Error setting value for key \"{}\": key cannot end with a period", key);
            return;
        }
        (*mLua)["__REG_BUF"] = value;
        sol::protected_function_result res = mLua->safe_script(std::format("{}.{} = __REG_BUF", mGlobalName, key));
        if (!res.valid()) {
            const sol::error err = res;
            Out("BaseRegistry", "Error setting value for key \"{}\": \"{}\"", key, err.what());
        }
        (*mLua)["__REG_BUF"] = sol::lua_nil;
    }

    template <typename T>
    std::optional<T> GetKeyValue(const std::string &key) {
        if (key.empty()) {
            Out("BaseRegistry", "Error getting value for key \"{}\": key is empty", key);
            return std::nullopt;
        }
        if (key.starts_with('.')) {
            Out("BaseRegistry", "Error getting value for key \"{}\": key cannot start with a period", key);
            return std::nullopt;
        }
        if (key.ends_with('.')) {
            Out("BaseRegistry", "Error getting value for key \"{}\": key cannot end with a period", key);
            return std::nullopt;
        }
        std::optional<sol::object> obj = RawGetKeyObject(key);
        if (!obj.has_value() || !obj->is<T>()) {
            return std::nullopt;
        }
        return obj->as<T>();
    }

    // Seeds a default. A key already holding a value of the expected type is left alone; one holding
    // the wrong type is overwritten, so a hand-edited registry with a broken value repairs itself.
    template <typename T>
    void SetKeyValueIfNotSet(const std::string &key, T value) {
        if (key.empty())
            return;
        if (std::optional<sol::object> existing = RawGetKeyObject(key);
            existing.has_value() && existing->is<T>())
            return;
        SetKeyValue<T>(key, value);
    }

    std::string GetGlobalName();
protected:
    std::string ApplyKeyComments(const std::string &serialized);

    std::string             mGlobalName;
    std::string             mLastError;
    std::filesystem::path   mFilePath;
    LuaState*               mLua;
    
    std::map<std::string, std::string> mKeyComments;

    std::mutex AccessRegistryMutex;
};
}