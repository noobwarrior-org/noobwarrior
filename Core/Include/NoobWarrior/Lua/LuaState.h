// === noobWarrior ===
// File: LuaState.h
// Started by: Hattozo
// Started on: 12/3/2025
// Description:
#pragma once
#include <lua.hpp>
#include <lauxlib.h>
#include <lualib.h>

#include "PluginEnv.h"
#include "VfsBinding.h"

namespace NoobWarrior {
enum class LuaContext {
    NoPlugin,
    InstallPlugin,
    UserPlugin,
};
class LuaState {
public:
    LuaState();
    int Open();
    void Close();
    lua_State* Get();
private:
    lua_State *L;

    PluginEnv mPluginEnv;
    VfsBinding mVfsBinding;
};
}