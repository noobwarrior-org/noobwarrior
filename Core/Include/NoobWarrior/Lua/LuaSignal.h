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
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include <lua.h>
#include <sol/sol.hpp>

#include <NoobWarrior/Log.h>
#include <NoobWarrior/Lua/LuaScript.h>

namespace NoobWarrior {
struct LuaSignalConnection {
    uint64_t Id { 0 };
    LuaScript* OwnerScript { nullptr };
    sol::protected_function Function;
};

using LuaSignalConnectionList = std::vector<LuaSignalConnection>;

// Handed to Lua by value, so copies of it come and go freely. It only refers to its connection by id
// and does nothing when it is destroyed; a connection lasts until Disconnect() or the signal's end.
class LuaSignalListener {
    friend class LuaSignal;
public:
    void Disconnect();
protected:
    std::weak_ptr<LuaSignalConnectionList> mConnections;
    uint64_t mId { 0 };
};

class LuaSignal {
public:
    LuaSignal();

    template<typename... Args>
    void Fire(Args... args) {
        // Iterate a copy: a listener may connect or disconnect listeners while it runs.
        const LuaSignalConnectionList snapshot = *mConnections;
        for (const LuaSignalConnection &connection : snapshot) {
            if (!IsConnected(connection.Id))
                continue;
            sol::protected_function_result res = connection.Function(args...);
            if (!res.valid())
                ReportError(connection, res);
        }
    }

    void LuaFire(sol::variadic_args args);
    LuaSignalListener Connect(sol::this_environment tenv, sol::protected_function func);
    void DisconnectAll();
protected:
    bool IsConnected(uint64_t id) const;
    static void ReportError(const LuaSignalConnection &connection, sol::protected_function_result &res);

    std::shared_ptr<LuaSignalConnectionList> mConnections;
    uint64_t mNextId { 1 };
};
}
