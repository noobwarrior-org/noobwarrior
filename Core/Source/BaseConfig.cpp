// === noobWarrior ===
// File: BaseConfig.cpp
// Started by: Hattozo
// Started on: 6/23/2025
// Description:
#include <NoobWarrior/BaseConfig.h>
#include <NoobWarrior/NoobWarrior.h>

#include <utility>

#include "lua/config_metatable.lua.inc"
#include "lua/serpent.lua.inc"

static int custom_serializer_func(lua_State *L) {
    const char *tag = luaL_checkstring(L, 1);
    const char *head = luaL_checkstring(L, 2);
    const char *body = luaL_checkstring(L, 3);
    const char *tail = luaL_checkstring(L, 4);

    lua_pushstring(L, (std::string(tag) + std::string(head) + std::string(body) + std::string(tail) + " -- Hey guys").c_str());

    return 1;
}

NoobWarrior::BaseConfig::BaseConfig(std::string globalName, std::filesystem::path filePath, lua_State *luaState) :
    mGlobalName(std::move(globalName)),
    mFilePath(std::move(filePath)),
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

    // Attach a metatable to our config table that will make it so that if you index anything in it, it will make it a table if its nil.
    // This is really good for doing assignments that require access to a lot of tables like "config.gui.database_editor.content_browser.size.x = 200"
    // because it will auto-create each table in the process without having to manually do it yourself.
    //
    // Of course this can get messy, so when we serialize the table we remove any empty tables beforehand so that it
    // doesn't look godawful when you open it in your text editor
    char buf[2048];
    snprintf(buf, 2048, config_metatable_lua, mGlobalName.c_str());
    luaL_dostring(mLuaState, buf);

    if (lua_isstring(mLuaState, -1)) {
        // uh oh, error message was pushed onto the stack
        mLastError = lua_tostring(mLuaState, -1);
        lua_pop(mLuaState, 1);
        return ConfigResponse::ErrorDuringExecution;
    }

    Out("Config", "Opened config");
    return ConfigResponse::Success;
}

NoobWarrior::ConfigResponse NoobWarrior::BaseConfig::Close() {
    // First lets remove all empty tables in our config table.
    // Since our metatable will automatically set any indexed nil value to a table, it creates a lot of clutter and junk
    std::string pruneSrc = std::format(R"(
        local function prune(tbl)
            for k, v in pairs(tbl) do
                if type(v) == "table" then
                    prune(v)
                    if next(v) == nil then
                        rawset(tbl, k, nil)
                    end
                end
            end
        end
        prune({});
    )", mGlobalName);

    if (int err = luaL_dostring(mLuaState, pruneSrc.c_str()); err != LUA_OK) {
        mLastError = lua_tostring(mLuaState, -1);
        lua_pop(mLuaState, 1);
        return ConfigResponse::ErrorDuringExecution;
    }

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
        // lua_pushcfunction(mLuaState, custom_serializer_func);
            // lua_setfield(mLuaState, -2, "custom");


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

    Out("Config", "Closed config");

    return ConfigResponse::Success;
}

std::string NoobWarrior::BaseConfig::GetLuaError() {
    return mLastError;
}

void NoobWarrior::BaseConfig::SetKeyComment(const char *key, const char *comment) {

}
