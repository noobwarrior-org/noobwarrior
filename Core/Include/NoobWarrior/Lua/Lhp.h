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
// File: Lhp.h
// Started by: Hattozo
// Started on: 1/9/2026
// Description: LHP (Lua Hypertext Preprocessor)
// It's like PHP but for Lua
#pragma once
#include <NoobWarrior/Url.h>
#include <sol/sol.hpp>
#include <string>

namespace NoobWarrior {
class LuaState;
class Lhp {
public:
    enum class RenderResponse {
        Failed,
        Success,
        SyntaxError,
        UrlFailed,
        LuaError,
        ExitCalled
    };

    Lhp(LuaState* lua);
    RenderResponse Render(sol::environment env, const std::string &input, std::string *output, Url path = {}, bool isRecursive = false);
    RenderResponse Render(sol::environment env, const Url &url, std::string *output, bool isRecursive = false);
private:
    LuaState* mLua;
};
}