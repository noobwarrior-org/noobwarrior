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
// File: LuaSignal.h
// Started by: Hattozo
// Started on: 2/19/2026
// Description:
#pragma once
#include <functional>
#include <vector>

#include <lua.hpp>
#include <sol/sol.hpp>

#include <NoobWarrior/Log.h>
#include <NoobWarrior/Lua/LuaScript.h>

namespace NoobWarrior {
class LuaSignal;
class LuaSignalListener {
    friend class LuaSignal;
public:
    LuaSignalListener(LuaSignal& parent);
    ~LuaSignalListener();

    void Disconnect();
protected:
    LuaScript* OwnerScript { nullptr };
    LuaSignal& Parent;
    sol::protected_function Function;
};

class LuaSignal {
    friend class LuaSignalListener;
public:
    LuaSignal();

    template<typename... Args>
    void Fire(Args... args) {
        for (LuaSignalListener &listener : mListeners) {
            sol::protected_function_result res = listener.Function(std::forward<Args>(args)...);
            if (!res.valid()) {
                sol::error err = res;
                Out("LuaScript", "[{}] (Execution Failure in Signal Listener) {}",
                    listener.OwnerScript != nullptr ? listener.OwnerScript->GetUrl().Resolve() : "unknown", err.what());
            }
        }
    }

    void LuaFire(sol::variadic_args args);

    LuaSignalListener Connect(sol::this_environment tenv, sol::protected_function func);
protected:
    std::vector<LuaSignalListener> mListeners;
};
}