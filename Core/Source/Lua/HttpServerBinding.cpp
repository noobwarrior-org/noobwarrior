// === noobWarrior ===
// File: HttpServerBinding.cpp
// Started by: Hattozo
// Started on: 1/4/2026
// Description:
#include <NoobWarrior/Lua/HttpServerBinding.h>
#include <NoobWarrior/HttpServer/Base/HttpServer.h>

using namespace NoobWarrior;

static int HttpServer_new(lua_State *L) {
    auto core = (Core*)lua_topointer(L, -1);
    HttpServer *srv = (HttpServer *)lua_newuserdata(L, sizeof(HttpServer));
    new(srv) HttpServer(core);
    lua_getmetatable(L, )
    return 1;
}

static int HttpServer_gc(lua_State *L) {
    
}

const luaL_Reg Lua::HttpServerFuncs[2] = {
    {"new", HttpServer_new},
    {nullptr, nullptr}
};

static const luaL_Reg HttpServerMetaFuncs[] = {
    {"__gc", HttpServer_gc},
    {NULL, NULL}
};