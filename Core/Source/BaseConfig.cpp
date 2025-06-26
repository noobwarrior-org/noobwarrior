// === noobWarrior ===
// File: BaseConfig.cpp
// Started by: Hattozo
// Started on: 6/23/2025
// Description:
#include <NoobWarrior/BaseConfig.h>
#include <NoobWarrior/NoobWarrior.h>

NoobWarrior::BaseConfig::BaseConfig(const std::filesystem::path &filePath, lua_State *luaState) :
    mFilePath(filePath),
    mFileOutput(nullptr),
    mLuaState(luaState)
{
    SerializeProperty<bool>(
        &DisableOverwriting,
        "meta.disable_overwriting",
        "Disables the overwriting of this config file every time the program is closed."
    );
}

NoobWarrior::ConfigResponse NoobWarrior::BaseConfig::ReadFromFile() {
    mLastError = "";

    if (!std::filesystem::exists(mFilePath))
        return ConfigResponse::Success; // Ignore this function if our file doesn't exist. It's fine, we have defaults to fall back to.

    int res = 0;

    res = luaL_loadfile(mLuaState, reinterpret_cast<const char*>(mFilePath.c_str()));
    if (res != LUA_OK) { mLastError = lua_tostring(mLuaState, -1); lua_pop(mLuaState, 1); }
    switch (res) {
    case LUA_OK: break;
    case LUA_ERRSYNTAX: return ConfigResponse::SyntaxError;
    case LUA_ERRMEM: return ConfigResponse::MemoryError;
    case LUA_ERRFILE: return ConfigResponse::CantReadFile;
    default: return ConfigResponse::Failed;
    }

    res = lua_pcall(mLuaState, 0, 1, 0);

    if (res != LUA_OK) {
        mLastError = lua_tostring(mLuaState, -1);
        lua_pop(mLuaState, 1);
        return ConfigResponse::ErrorDuringExecution;
    }

    if (!lua_istable(mLuaState, -1)) {
        lua_getglobal(mLuaState, "error");
        lua_pushstring(mLuaState, std::format("expected table, got {}", lua_typename(mLuaState, lua_type(mLuaState, -2))).c_str());
        lua_pcall(mLuaState, 1, 0, 0);
        mLastError = lua_tostring(mLuaState, -1);
        lua_pop(mLuaState, 1);
        return ConfigResponse::ReturningWrongType;
    }

    lua_setglobal(mLuaState, "config");

    // luaL_newmetatable(mLuaState, )
    // lua_setmetatable(mLuaState, -2);

    /*for (const auto &[prop, keyAndComment] : mSerializedProperties) {
        bool failed = false;
        int stackSize = 0;
        char *token = strtok(const_cast<char*>(keyAndComment.first), ".");
        while (token != nullptr) {
            stackSize++;
            lua_getfield(mLuaState, -1, token);
            token = strtok(nullptr, " ");
            // so we haven't yet reached the end of our tokenizer, but somehow this is not a table? that's not right
            if (token != nullptr && !lua_istable(mLuaState, -1)) {
                luaL_error(mLuaState, R"(expected type "table" for token "%s", got "%s")", token, lua_typename(mLuaState, lua_type(mLuaState, -1)));
                lua_pop(mLuaState, stackSize);
                failed = true;
                break;
            }
        }
        if (failed) continue;
        if (lua_isnil(mLuaState, -1))
            goto done;
        switch (prop.type()) {
        case typeid(std::string):
            if (lua_type(mLuaState, -1) == LUA_TSTRING)
                *std::any_cast<std::string*>(prop) = std::string(lua_tostring(mLuaState, -1));
            break;
        case typeid(bool):
            if (lua_type(mLuaState, -1) == LUA_TBOOLEAN)
                *std::any_cast<bool*>(prop) = static_cast<bool>(lua_toboolean(mLuaState, -1));
            break;
        case typeid(int):
            if (lua_type(mLuaState, -1) == LUA_TNUMBER)
                *std::any_cast<int*>(prop) = static_cast<int>(lua_tointeger(mLuaState, -1));
            break;
        default: break;
        }
        done:
            lua_pop(mLuaState, stackSize);
    }*/

    OnDeserialize();
    DeserializeProperty<bool>(&DisableOverwriting, "meta.disable_overwriting");

    lua_pop(mLuaState, 1);
    return ConfigResponse::Success;
}

NoobWarrior::ConfigResponse NoobWarrior::BaseConfig::WriteToFile() {
    if (mFileOutput != nullptr) { NOOBWARRIOR_FREE_PTR(mFileOutput) }

    mFileOutput = new std::ofstream(mFilePath);
    if (mFileOutput->fail())
        return ConfigResponse::CantReadFile;

    *mFileOutput << "return {\n";
        OnSerialize();
        SerializeProperty<bool>(DisableOverwriting, "meta.disable_overwriting",
            "Disables the overwriting of this config file every time the program is closed.");
    *mFileOutput << "}";

    NOOBWARRIOR_FREE_PTR(mFileOutput)

    return ConfigResponse::Success;
}

std::string NoobWarrior::BaseConfig::GetLuaError() {
    return mLastError;
}