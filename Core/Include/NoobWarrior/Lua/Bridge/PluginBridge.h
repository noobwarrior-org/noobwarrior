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
// File: PluginBridge.h
// Started by: Hattozo
// Started on: 1/17/2026
// Description:
#pragma once
#include <NoobWarrior/Lua/Bridge/LuaObjectBridge.h>

namespace NoobWarrior {
class Plugin;
// little lightweight wrapper for plugin, so lua can memory manage this class instead of our real plugins.
class PluginWrapper {
public:
    PluginWrapper(Plugin* realPlugin);
private:
    Plugin* mPlugin;
};

class PluginBridge : public LuaObjectBridge {
public:
    PluginBridge(LuaState* lua);
    LuaReg GetStaticFuncs() override;
    LuaReg GetObjectMetaFuncs() override;
    LuaReg GetObjectFuncs() override;
};
}