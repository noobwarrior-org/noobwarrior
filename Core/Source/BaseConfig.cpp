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
// File: BaseConfig.cpp
// Started by: Hattozo
// Started on: 6/23/2025
// Description:
#include <NoobWarrior/BaseConfig.h>
#include <NoobWarrior/NoobWarrior.h>

#include <utility>

#include "Lua/files/config_metatable.lua.inc.cpp"
#include "Lua/files/serpent.lua.inc.cpp"

static int custom_serializer_func(lua_State *L) {
    const char *tag = luaL_checkstring(L, 1);
    const char *head = luaL_checkstring(L, 2);
    const char *body = luaL_checkstring(L, 3);
    const char *tail = luaL_checkstring(L, 4);

    lua_pushstring(L, (std::string(tag) + std::string(head) + std::string(body) + std::string(tail) + " -- Hey guys").c_str());

    return 1;
}

NoobWarrior::BaseConfig::BaseConfig(std::string globalName, std::filesystem::path filePath, LuaState* lua) :
    mGlobalName(std::move(globalName)),
    mFilePath(std::move(filePath)),
    mFileOutput(nullptr),
    mLua(lua)
{}

NoobWarrior::ConfigResponse NoobWarrior::BaseConfig::Open() {
    lua_State* L = mLua->Get();
    mLastError = "";

    if (!std::filesystem::exists(mFilePath))
        lua_newtable(L); // file doesn't exist, create a new empty table.
    else {
        int res = 0;

        res = luaL_loadfile(L, reinterpret_cast<const char*>(mFilePath.generic_string().c_str()));
        if (res != LUA_OK) { mLastError = lua_tostring(L, -1); lua_pop(L, 1); }
        switch (res) {
            case LUA_OK: break;
            case LUA_ERRSYNTAX: return ConfigResponse::SyntaxError;
            case LUA_ERRMEM: return ConfigResponse::MemoryError;
            case LUA_ERRFILE: return ConfigResponse::CantReadFile;
            default: return ConfigResponse::Failed;
        }

        res = lua_pcall(L, 0, 1, 0);

        if (res != LUA_OK) {
            mLastError = lua_tostring(L, -1);
            lua_pop(L, 1);
            return ConfigResponse::ErrorDuringExecution;
        }

        if (!lua_istable(L, -1)) {
            lua_getglobal(L, "error");
            lua_pushstring(L, std::format("expected table, got {}", lua_typename(L, lua_type(L, -2))).c_str());
            res = lua_pcall(L, 1, 0, 0);
            mLastError = lua_tostring(L, -1);
            lua_pop(L, 2);
            return ConfigResponse::ReturningWrongType;
        }
    }

    lua_setglobal(L, mGlobalName.c_str()); // set our global as what our config file returned, a table. also pops it off the stack

    // Attach a metatable to our config table that will make it so that if you index anything in it, it will make it a table if its nil.
    // This is really good for doing assignments that require access to a lot of tables like "config.gui.database_editor.content_browser.size.x = 200"
    // because it will auto-create each table in the process without having to manually do it yourself.
    //
    // Of course this can get messy, so when we serialize the table we remove any empty tables beforehand so that it
    // doesn't look godawful when you open it in your text editor
    char buf[2048];
    snprintf(buf, 2048, config_metatable_lua, mGlobalName.c_str());
    luaL_dostring(L, buf);

    if (lua_isstring(L, -1)) {
        // uh oh, error message was pushed onto the stack
        mLastError = lua_tostring(L, -1);
        lua_pop(L, 1);
        return ConfigResponse::ErrorDuringExecution;
    }

    Out("Config", "Opened config");
    return ConfigResponse::Success;
}

NoobWarrior::ConfigResponse NoobWarrior::BaseConfig::Close() {
    lua_State* L = mLua->Get();
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

    if (int err = luaL_dostring(L, pruneSrc.c_str()); err != LUA_OK) {
        mLastError = lua_tostring(L, -1);
        lua_pop(L, 1);
        return ConfigResponse::ErrorDuringExecution;
    }

    // Load in serpent.lua through our header file that embeds it into the program.
    // It's a lua serializer that reconstructs a source-code version of the table.
    int res = luaL_loadstring(L, serpent_lua);
    if (res != LUA_OK) { mLastError = lua_tostring(L, -1); lua_pop(L, 1); }
    switch (res) {
        case LUA_OK: break;
        case LUA_ERRSYNTAX: return ConfigResponse::SyntaxError;
        case LUA_ERRMEM: return ConfigResponse::MemoryError;
        case LUA_ERRFILE: return ConfigResponse::CantReadFile;
        default: return ConfigResponse::Failed;
    }

    res = lua_pcall(L, 0, 1, 0);

    if (res != LUA_OK) {
        mLastError = lua_tostring(L, -1);
        lua_pop(L, 1);
        return ConfigResponse::ErrorDuringExecution;
    }

    // now that we have serpent.lua on our stack, access one of the functions it has (block) and add our config global
    // as the arguments. and then call it.
    lua_getfield(L, -1, "block");

    lua_getglobal(L, mGlobalName.c_str()); // arg 1: our global config table

    lua_newtable(L); // arg 2: options table
        lua_pushstring(L, "    ");
            lua_setfield(L, -2, "indent");
        // disable the printing of addresses of tables because it's unnecessary.
        lua_pushboolean(L, false);
            lua_setfield(L, -2, "comment");
        // lua_pushcfunction(mLuaState, custom_serializer_func);
            // lua_setfield(mLuaState, -2, "custom");


    res = lua_pcall(L, 2, 1, 0);

    if (res != LUA_OK) {
        mLastError = lua_tostring(L, -1);
        lua_pop(L, 2); // you still have this error message and the serpent.lua table on the stack so pop those 2
        return ConfigResponse::ErrorDuringExecution;
    }

    const char *serializedConfigTable = lua_tostring(L, -1); // now we have our serialized table

    // open file and write to it
    if (mFileOutput != nullptr) { NOOBWARRIOR_FREE_PTR(mFileOutput) }
    mFileOutput = new std::ofstream(mFilePath);
    if (mFileOutput->fail())
        return ConfigResponse::CantReadFile;
    *mFileOutput << "return " << serializedConfigTable;
    NOOBWARRIOR_FREE_PTR(mFileOutput)

    lua_pop(L, 2); // pop the string and table

    Out("Config", "Closed config");

    return ConfigResponse::Success;
}

std::string NoobWarrior::BaseConfig::GetLuaError() {
    return mLastError;
}

void NoobWarrior::BaseConfig::SetKeyComment(const char *key, const char *comment) {

}

std::string NoobWarrior::BaseConfig::ConvertJsonStrToLuaStr(const std::string &jsonStr) {
    lua_State* L = mLua->Get();
    std::string str;
    std::string stmt = std::format("local jsonToTbl = json.parse('{}')\nlocal tblString = serpent.block(jsonToTbl)\nreturn tblString", jsonStr);
    int err = luaL_dostring(L, stmt.c_str());
    if (err)
        Out("Config", "Failed to convert JSON string to Lua table string: {}", lua_tostring(L, -1));
    else
        str = lua_tostring(L, -1);
    lua_pop(L, 1);
    return str;
}
