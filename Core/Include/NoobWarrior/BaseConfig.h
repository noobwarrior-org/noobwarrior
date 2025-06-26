// === noobWarrior ===
// File: BaseConfig.h
// Started by: Hattozo
// Started on: 6/23/2025
// Description:
#pragma once

#include <lua.hpp>
#include <filesystem>
#include <fstream>
#include <map>
#include <any>

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
    BaseConfig(const std::filesystem::path &filePath, lua_State *luaState);
    ConfigResponse ReadFromFile();
    ConfigResponse WriteToFile();
    std::string GetLuaError();

    bool DisableOverwriting;
protected:
    virtual void OnDeserialize() = 0;
    virtual void OnSerialize() = 0;

    std::string             mLastError;
    std::filesystem::path   mFilePath;
    std::ostream*           mFileOutput;
    lua_State*              mLuaState;

    /*std::map<std::any, std::pair<const char*, const char*>> mSerializedProperties;

    template <typename T>
    void SerializeProperty(T *property, const char *key, const char *comment = nullptr) {
        mSerializedProperties[property] = { key, comment };
    }*/

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
};
}