// === noobWarrior ===
// File: LuaState.cpp
// Started by: Hattozo
// Started on: 12/3/2025
// Description:
#include <NoobWarrior/Lua/LuaState.h>
#include <NoobWarrior/Lua/HttpServerBindings.h>
#include <NoobWarrior/Log.h>

#include "files/global_env_metatable.lua.inc"
#include "files/rawget_path.lua.inc"
#include "files/serpent.lua.inc"
#include "files/json.lua.inc"

using namespace NoobWarrior;

static int printBS(lua_State *L) {
    const char *str = luaL_checkstring(L, 1);
    Out("Lua", str);
    return 0;
}

LuaState::LuaState() : L(nullptr) {}

int LuaState::Open() {
    L = luaL_newstate();

    luaL_openlibs(L);

    lua_pushcfunction(L, printBS);
    lua_setglobal(L, "print");

    // lua_pushcfunction(mLuaState, printBS);
    // lua_setglobal(mLuaState, "error");

    // Run lua code to define some functions without having to hassle with Lua C API
    luaL_dostring(L, rawget_path_lua);
    luaL_dostring(L, global_env_metatable_lua);

#define LOADLIBRARY(strVar, name) \
    int strVar##_exec_res = luaL_dostring(L, strVar); \
    if (!strVar##_exec_res && lua_istable(L, -1)) { \
        lua_setglobal(L, name); \
    } else lua_pop(L, 1);
    
    LOADLIBRARY(serpent_lua, "serpent")
    LOADLIBRARY(json_lua, "json")

#undef LOADLIBRARY

    luaL_newlib(L, HttpServerFuncs);
    lua_setglobal(L, "HttpServer");

    Out("Lua", "Initialized Lua");
    return 1;
}

void LuaState::Close() {
    lua_close(L);
}

lua_State* LuaState::Get() {
    return L;
}