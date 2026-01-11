// === noobWarrior ===
// File: Lhp.h
// Started by: Hattozo
// Started on: 1/9/2026
// Description: LHP (Lua Hypertext Preprocessor)
// It's like PHP but for Lua
#pragma once
#include "LuaState.h"

#include <string>

namespace NoobWarrior {
namespace Lhp {
enum class RenderResponse {
    Failed,
    Success,

};

RenderResponse Render(LuaState* lua, const std::string &input, std::string *output);
}
}