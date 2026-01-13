// === noobWarrior ===
// File: LuaHypertextPreprocessor.cpp
// Started by: Hattozo
// Started on: 1/9/2026
// Description: LHP (Lua Hypertext Preprocessor)
// It's like PHP but for Lua
#include <NoobWarrior/Lua/LuaHypertextPreprocessor.h>
#include <NoobWarrior/Lua/LuaState.h>
#include <NoobWarrior/NoobWarrior.h>

#include <lua.h>

#include "files/lhp_env_metatable.lua.inc.cpp"

#define OPENING_TAG "<?lua"
#define CLOSING_TAG "?>"

using namespace NoobWarrior;

LuaHypertextPreprocessor::LuaHypertextPreprocessor(LuaState* lua) : mLua(lua) {

}

LuaHypertextPreprocessor::RenderResponse LuaHypertextPreprocessor::Render(const std::string &input, std::string *output) {
    bool lua_mode = false;
    for (int i = 0; i < input.size(); i++) {
        if (input.substr(i, NOOBWARRIOR_ARRAY_SIZE(OPENING_TAG) - 1) == OPENING_TAG) {
            // Switch to Lua mode, skip cursor to the first letter after the tag and restart
            lua_mode = true;
            i += NOOBWARRIOR_ARRAY_SIZE(OPENING_TAG);
            continue;
        }

        if (input.substr(i, NOOBWARRIOR_ARRAY_SIZE(CLOSING_TAG) - 1) == CLOSING_TAG) {
            lua_mode = false;
            i += NOOBWARRIOR_ARRAY_SIZE(CLOSING_TAG);
            continue;
        }

        if (!lua_mode)
            output += input.at(i);
        else {
            lua_State *L = lua->Get();
            
            

            lua_setfenv(L, -1);
            luaL_dostring()
            luaL_loadstring(L, )
        }
    }
}