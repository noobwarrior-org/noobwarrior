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
// File: LuaSignal.cpp
// Started by: Hattozo
// Started on: 2/19/2026
// Description:
#include <NoobWarrior/Lua/LuaSignal.h>
#include <NoobWarrior/Log.h>

using namespace NoobWarrior;

LuaSignalListener::LuaSignalListener(LuaSignal& parent) : Parent(parent) {
    
}

LuaSignalListener::~LuaSignalListener() {
    Disconnect();
}

void LuaSignalListener::Disconnect() {
    // auto it = std::find(Parent.mListeners.begin(), Parent.mListeners.end(), *this);
    // if (it != Parent.mListeners.end())
    //     Parent.mListeners.erase(it);
}

LuaSignal::LuaSignal() {

}

LuaSignalListener LuaSignal::Connect(sol::this_environment tenv, sol::protected_function func) {
    sol::environment env(tenv);

    LuaSignalListener listener(*this);
    listener.Function = func;
    listener.OwnerScript = env["script"].get_or<LuaScript*>(nullptr);

    mListeners.push_back(std::move(listener));
    return listener;
}

void LuaSignal::LuaFire(sol::variadic_args args) {
    for (LuaSignalListener &listener : mListeners) {
        listener.Function(args);
    }
}