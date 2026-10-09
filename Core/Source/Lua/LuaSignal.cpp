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

#include <algorithm>

using namespace NoobWarrior;

void LuaSignalListener::Disconnect() {
    std::shared_ptr<LuaSignalConnectionList> connections = mConnections.lock();
    if (connections == nullptr)
        return;
    std::erase_if(*connections, [this](const LuaSignalConnection &connection) {
        return connection.Id == mId;
    });
}

LuaSignal::LuaSignal() : mConnections(std::make_shared<LuaSignalConnectionList>()) {

}

LuaSignalListener LuaSignal::Connect(sol::this_environment tenv, sol::protected_function func) {
    sol::environment env(tenv);

    LuaSignalConnection connection;
    connection.Id = mNextId++;
    connection.Function = func;
    connection.OwnerScript = env["script"].get_or<LuaScript*>(nullptr);
    mConnections->push_back(std::move(connection));

    LuaSignalListener listener;
    listener.mConnections = mConnections;
    listener.mId = mConnections->back().Id;
    return listener;
}

void LuaSignal::LuaFire(sol::variadic_args args) {
    const LuaSignalConnectionList snapshot = *mConnections;
    for (const LuaSignalConnection &connection : snapshot) {
        if (!IsConnected(connection.Id))
            continue;
        sol::protected_function_result res = connection.Function(args);
        if (!res.valid())
            ReportError(connection, res);
    }
}

void LuaSignal::DisconnectAll() {
    mConnections->clear();
}

bool LuaSignal::IsConnected(uint64_t id) const {
    return std::any_of(mConnections->begin(), mConnections->end(), [id](const LuaSignalConnection &connection) {
        return connection.Id == id;
    });
}

void LuaSignal::ReportError(const LuaSignalConnection &connection, sol::protected_function_result &res) {
    sol::error err = res;
    Out("LuaScript", "[{}] (Execution Failure in Signal Listener) {}",
        connection.OwnerScript != nullptr ? connection.OwnerScript->GetUrl().Resolve() : "unknown", err.what());
}
