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
// File: LuaState.h
// Started by: Hattozo
// Started on: 12/3/2025
// Description:
#pragma once
#include <lua.hpp>
#include <sol/sol.hpp>

#include <NoobWarrior/Lua/LuaScript.h>
#include <NoobWarrior/Lua/Lhp.h>

#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace NoobWarrior {
class Core;
enum class LuaContext {
    NoPlugin,
    InstallPlugin,
    UserPlugin,
};
class LuaState : public sol::state {
public:
    LuaState(Core* core);
    int Open();
    bool Opened();

    lua_State* Get();
    Lhp *GetLhp();
    Core *GetCore();

    bool IsScriptLoading(const std::string& resolvedUrl);
    void MarkScriptLoading(const std::string& resolvedUrl);
    void UnmarkScriptLoading(const std::string& resolvedUrl);
    void CacheModule(const std::string& resolvedUrl, std::unique_ptr<LuaScript> module);
    LuaScript* RetrieveCachedModuleFromResolvedUrl(const std::string& resolvedUrl) const;
private:
    Core* mCore;
    Lhp mLhp;

    std::unordered_set<std::string> mLoadingScripts;
    std::unordered_map<std::string, std::unique_ptr<LuaScript>> mCachedModules;
};
}