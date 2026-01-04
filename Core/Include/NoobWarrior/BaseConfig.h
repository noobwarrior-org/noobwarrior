// === noobWarrior ===
// File: BaseConfig.h
// Started by: Hattozo
// Started on: 6/23/2025
// Description:
#pragma once
#include <NoobWarrior/Log.h>

#include <NoobWarrior/Lua/LuaState.h>
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
        AccessConfigMutex.lock();
        lua_State* L = mLua->Get();
        // BTW: std::format() will automatically render bools as "true" or "false" in textual form, so you won't have to hassle with that.
        std::string str;
        if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, char*> || std::is_same_v<T, const char*>)
            str = std::format("{}.{} = {}", mGlobalName, key, std::format("\"{}\"", value).c_str());
        else if constexpr (std::is_same_v<T, std::nullopt_t>)
            str = std::format("{}.{} = nil", mGlobalName, key);
        else if constexpr (std::is_same_v<T, nlohmann::json>)
            str = std::format("{}.{} = {}", mGlobalName, key, ConvertJsonStrToLuaStr(value.dump()));
        else
            str = std::format("{}.{} = {}", mGlobalName, key, value);
        luaL_dostring(L, str.c_str());
        AccessConfigMutex.unlock();
    }

    template <typename T>
    std::optional<T> GetKeyValue(const std::string &key) {
        AccessConfigMutex.lock();
        lua_State* L = mLua->Get();
        luaL_dostring(L, std::format("return rawget_path({}, \"{}\")", mGlobalName, key).c_str());

        if (lua_isnil(L, -1)) {
            AccessConfigMutex.unlock();
            return std::nullopt;
        }

        T result {};
        // sorry for the ugly code
        if constexpr (std::is_same_v<T, std::string>)
            result = lua_type(L, -1) == LUA_TSTRING ? std::string(lua_tostring(L, -1)) : "";
        else if constexpr (std::is_same_v<T, char*> || std::is_same_v<T, const char*>)
            result = lua_type(L, -1) == LUA_TSTRING ? lua_tostring(L, -1) : "";
        else if constexpr (std::is_same_v<T, int> || std::is_enum_v<T>)
            result = lua_type(L, -1) == LUA_TNUMBER ? static_cast<T>(lua_tointeger(L, -1)) : 0;
        else if constexpr (std::is_same_v<T, double> || std::is_same_v<T, float>)
            result = lua_type(L, -1) == LUA_TNUMBER ? static_cast<T>(lua_tonumber(L, -1)) : 0;
        else if constexpr (std::is_same_v<T, bool>)
            result = lua_type(L, -1) == LUA_TBOOLEAN ? lua_toboolean(L, -1) : false;
        else if constexpr (std::is_same_v<T, nlohmann::json>) {
            if (lua_type(L, -1) == LUA_TTABLE) {
                lua_getglobal(L, "json");
                lua_getfield(L, -1, "stringify");

                lua_pushvalue(L, -3); // retrieve the key value and push it to the stack again

                int res = lua_pcall(L, 1, 1, 0);

                if (res != LUA_OK) {
                    Out("Config", "Failed to convert Lua table to JSON string: {}", lua_tostring(L, -1));
                }
                try {
                    result = nlohmann::json::parse(lua_tostring(L, -1));
                } catch (std::exception &ex) {
                    Out("Config", "Failed to convert string to C++ JSON object");
                }
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 1);
        AccessConfigMutex.unlock();
        return result;
    }

    template <typename T>
    void SetKeyValueIfNotSet(const std::string &key, T value) {
        if (!GetKeyValue<T>(key).has_value())
            SetKeyValue<T>(key, value);
    }

    std::string ConvertJsonStrToLuaStr(const std::string &jsonStr);
protected:
    std::string             mGlobalName;
    std::string             mLastError;
    std::filesystem::path   mFilePath;
    std::ostream*           mFileOutput;
    LuaState*               mLua;

    std::mutex AccessConfigMutex;

    class Namespace {
    public:
        Namespace();

    };

    Namespace &GetNamespace(const std::string &namespaceStr);

    /*std::map<std::any, std::pair<const char*, const char*>> mSerializedProperties;

    template <typename T>
    void SerializeProperty(T *property, const char *key, const char *comment = nullptr) {
        mSerializedProperties[property] = { key, comment };
    }*/

    /*
    template <typename T>
    void DeserializeProperty(T *property, const char *key) {
        int stackSize = 0;
        char *token = strtok(const_cast<char*>(key), ".");
        while (token != nullptr) {
            stackSize++;
            lua_getfield(mLuaState, -1, token);
            token = strtok(nullptr, " ");
            // so we haven't yet reached the end of our tokenizer, but somehow this is not a table? that's not right
            if (token != nullptr && !lua_istable(mLuaState, -1)) {
                luaL_error(mLuaState, R"(expected type "table" for token "%s", got "%s")", token, lua_typename(mLuaState, lua_type(mLuaState, -1)));
                lua_pop(mLuaState, stackSize);
                return;
            }
        }
        if (lua_isnil(mLuaState, -1))
            goto done;
        if constexpr (std::is_same_v<T, std::string>) {
            if (lua_type(mLuaState, -1) == LUA_TSTRING)
                *property = std::string(lua_tostring(mLuaState, -1));
        } else if constexpr (std::is_same_v<T, int> || std::is_enum_v<T>) {
            if (lua_type(mLuaState, -1) == LUA_TNUMBER)
                *property = static_cast<T>(lua_tointeger(mLuaState, -1));
        } else if constexpr (std::is_same_v<T, bool>) {
            if (lua_type(mLuaState, -1) == LUA_TBOOLEAN)
                *property = lua_toboolean(mLuaState, -1);
        }
    done:
        lua_pop(mLuaState, stackSize);
    }

    template <typename T>
    void SerializeProperty(T property, const char *key, const char *comment = nullptr) {
        int level = 0;
        char *token = strtok(const_cast<char*>(key), ".");
        while (token != nullptr) {
            level++;
            std::string spaces(level * 4, ' ');
            *mFileOutput << spaces << token << " = {\n" << spaces << "}";
            token = strtok(nullptr, " ");
            if (token != nullptr) {

            }
        }
    }
    */
};
}