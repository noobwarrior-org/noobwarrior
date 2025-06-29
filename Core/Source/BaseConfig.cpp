// === noobWarrior ===
// File: BaseConfig.cpp
// Started by: Hattozo
// Started on: 6/23/2025
// Description:
#include <NoobWarrior/BaseConfig.h>
#include <NoobWarrior/NoobWarrior.h>

#include "serpent.lua.inc"

static int custom_serializer_func(lua_State *L) {
    const char *tag = luaL_checkstring(L, 1);
    const char *head = luaL_checkstring(L, 2);
    const char *body = luaL_checkstring(L, 3);
    const char *tail = luaL_checkstring(L, 4);

    lua_pushstring(L, (std::string(tag) + std::string(head) + std::string(body) + std::string(tail) + " -- Hey guys").c_str());

    return 1;
}

NoobWarrior::BaseConfig::BaseConfig(const std::string &globalName, const std::filesystem::path &filePath, lua_State *luaState) :
    mGlobalName(globalName),
    mFilePath(filePath),
    mFileOutput(nullptr),
    mLuaState(luaState)
{}

NoobWarrior::ConfigResponse NoobWarrior::BaseConfig::Open() {
    mLastError = "";

    if (!std::filesystem::exists(mFilePath))
        lua_newtable(mLuaState); // file doesn't exist, create a new empty table.
    else {
        int res = 0;

        res = luaL_loadfile(mLuaState, reinterpret_cast<const char*>(mFilePath.generic_string().c_str()));
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
            res = lua_pcall(mLuaState, 1, 0, 0);
            mLastError = lua_tostring(mLuaState, -1);
            lua_pop(mLuaState, 2);
            return ConfigResponse::ReturningWrongType;
        }
    }

    lua_setglobal(mLuaState, mGlobalName.c_str()); // set our global as what our config file returned, a table. also pops it off the stack

    // not doing this from C, it gets too ugly
    char buf[1024];
    snprintf(buf, 1024, R"(
    -- metatable that, when a value is accessed, automatically sets that value as a table if it doesn't exist (nil)
    -- so you can do stuff like "config.this_property_doesnt_exist.this_property_also_doesnt_exist.my_setting = "hi"
    local metatable
    metatable = {
        __index = function(table, index)
            local value = rawget(table, index)
            if value == nil then
                local tbl = {}
                setmetatable(tbl, metatable) -- recursively do this :)
                rawset(table, index, tbl)
                return tbl
            else return value end
        end
    }
    setmetatable(%s, metatable)
    )", mGlobalName.c_str());
    luaL_dostring(mLuaState, buf);

    if (lua_isstring(mLuaState, -1)) {
        // uh oh, error message was pushed onto the stack
        mLastError = lua_tostring(mLuaState, -1);
        lua_pop(mLuaState, 1);
        return ConfigResponse::ErrorDuringExecution;
    }

    // now init our default key values
    SetKeyValue("meta.version", "v1.0.0");
    // wip

    return ConfigResponse::Success;
}

NoobWarrior::ConfigResponse NoobWarrior::BaseConfig::Close() {
    // Load in serpent.lua through our header file that embeds it into the program.
    // It's a lua serializer that reconstructs a source-code version of the table.
    int res = luaL_loadstring(mLuaState, serpent_lua);
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

    // now that we have serpent.lua on our stack, access one of the functions it has (block) and add our config global
    // as the arguments. and then call it.
    lua_getfield(mLuaState, -1, "block");

    lua_getglobal(mLuaState, mGlobalName.c_str()); // arg 1: our global config table

    lua_newtable(mLuaState); // arg 2: options table
        lua_pushstring(mLuaState, "    ");
            lua_setfield(mLuaState, -2, "indent");
        // disable the printing of addresses of tables because it's unnecessary.
        lua_pushboolean(mLuaState, false);
            lua_setfield(mLuaState, -2, "comment");
        lua_pushcfunction(mLuaState, custom_serializer_func);
            lua_setfield(mLuaState, -2, "custom");


    res = lua_pcall(mLuaState, 2, 1, 0);

    if (res != LUA_OK) {
        mLastError = lua_tostring(mLuaState, -1);
        lua_pop(mLuaState, 2); // you still have this error message and the serpent.lua table on the stack so pop those 2
        return ConfigResponse::ErrorDuringExecution;
    }

    const char *serializedConfigTable = lua_tostring(mLuaState, -1); // now we have our serialized table

    // open file and write to it
    if (mFileOutput != nullptr) { NOOBWARRIOR_FREE_PTR(mFileOutput) }
    mFileOutput = new std::ofstream(mFilePath);
    if (mFileOutput->fail())
        return ConfigResponse::CantReadFile;
    *mFileOutput << "return " << serializedConfigTable;
    NOOBWARRIOR_FREE_PTR(mFileOutput)

    lua_pop(mLuaState, 2); // pop the string and table

    return ConfigResponse::Success;
}

std::string NoobWarrior::BaseConfig::GetLuaError() {
    return mLastError;
}

void NoobWarrior::BaseConfig::SetKeyComment(const char *key, const char *comment) {

}
