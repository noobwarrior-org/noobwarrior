// === noobWarrior ===
// File: LuaState.h
// Started by: Hattozo
// Started on: 12/3/2025
// Description:
#pragma once
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

namespace NoobWarrior {
class LuaState {
public:
    LuaState();
    int Open();
    void Close();
    lua_State* Get();
private:
    lua_State *L;
};
}