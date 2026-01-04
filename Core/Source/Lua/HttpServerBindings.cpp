// === noobWarrior ===
// File: HttpServerBindings.cpp
// Started by: Hattozo
// Started on: 1/4/2026
// Description:
#include <NoobWarrior/Lua/HttpServerBindings.h>
#include <NoobWarrior/HttpServer/Base/HttpServer.h>

using namespace NoobWarrior;

static int HttpServer_new(lua_State *L) {
    HttpServer *srv = (HttpServer *)lua_newuserdata(L, sizeof(HttpServer));
    new(srv) HttpServer();
}

const luaL_Reg NoobWarrior::HttpServerFuncs[2] = {
    {"new", HttpServer_new},
    {nullptr, nullptr}
};