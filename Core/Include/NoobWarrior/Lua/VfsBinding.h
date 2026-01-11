// === noobWarrior ===
// File: VfsBinding.h
// Started by: Hattozo
// Started on: 1/10/2026
// Description:
#pragma once
#include <lauxlib.h>

namespace NoobWarrior {
    class LuaState;
    class VfsBinding {
    public:
        VfsBinding(LuaState* lua);
        void Open();
        void Close();
    private:
        LuaState* mLua;
    };
}