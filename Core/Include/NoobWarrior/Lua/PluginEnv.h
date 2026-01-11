// === noobWarrior ===
// File: PluginEnv.h
// Started by: Hattozo
// Started on: 1/10/2026
// Description:
#pragma once

namespace NoobWarrior {
class LuaState;
class PluginEnv {
public:
    PluginEnv(LuaState* lua);
    void Open();
    void Close();
    void Push();
protected:
    LuaState* mLua;
    int mRef;
};
}