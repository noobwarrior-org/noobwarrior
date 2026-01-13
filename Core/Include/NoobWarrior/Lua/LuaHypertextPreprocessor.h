// === noobWarrior ===
// File: LuaHypertextPreprocessor.h
// Started by: Hattozo
// Started on: 1/9/2026
// Description: LHP (Lua Hypertext Preprocessor)
// It's like PHP but for Lua
#pragma once
#include <string>

namespace NoobWarrior {
class LuaState;
class LuaHypertextPreprocessor {
public:
    enum class RenderResponse {
        Failed,
        Success,
    };

    LuaHypertextPreprocessor(LuaState* lua);
    RenderResponse Render(const std::string &input, std::string *output);
private:
    LuaState* mLua;
};
}