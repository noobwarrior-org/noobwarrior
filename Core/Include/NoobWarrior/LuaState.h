// === noobWarrior ===
// File: LuaState.h
// Started by: Hattozo
// Started on: 6/24/2025
// Description:
#pragma once
#include "lua.h"

namespace NoobWarrior {
class LuaState {
public:
    LuaState();
    lua_State *L;
};
}
