// === noobWarrior ===
// File: HttpServerBinding.cpp
// Started by: Hattozo
// Started on: 1/4/2026
// Description:
#include <NoobWarrior/Lua/Bridge/HttpServerBridge.h>
#include <NoobWarrior/HttpServer/Base/HttpServer.h>

using namespace NoobWarrior;

static int HttpServer_new(lua_State *L) {
    lua_getfield(L, LUA_REGISTRYINDEX, "core");
    auto core = (Core*)lua_topointer(L, -1);
    lua_pop(L, 1);

    HttpServer *srv = (HttpServer *)lua_newuserdata(L, sizeof(HttpServer));
    new(srv) HttpServer(core);

    luaL_setmetatable(L, "HttpServer");

    return 1;
}

static int HttpServer_gc(lua_State *L) {
    HttpServer* srv = (HttpServer*)luaL_checkudata(L, 1, "HttpServer");
    if (srv != nullptr) {
        delete srv;
    }
    return 0;
}

HttpServerBridge::HttpServerBridge(LuaState* lua) : LuaObjectBridge(lua, "HttpServer") {

}

LuaReg HttpServerBridge::GetStaticFuncs() {
    return {
        {"new", HttpServer_new}
    };
}

LuaReg HttpServerBridge::GetObjectMetaFuncs() {
    return {
        {"__gc", HttpServer_gc}
    };
}
