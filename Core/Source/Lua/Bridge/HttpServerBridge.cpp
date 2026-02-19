// === noobWarrior ===
// File: HttpServerBinding.cpp
// Started by: Hattozo
// Started on: 1/4/2026
// Description:
#include <NoobWarrior/Lua/Bridge/HttpServerBridge.h>
#include <NoobWarrior/HttpServer/Base/HttpServer.h>

using namespace NoobWarrior;

static int HttpServer_OnRequestCallback(lua_State *L) {
    HttpServer* udata = (HttpServer*)luaL_checkudata(L, lua_upvalueindex(1), "HttpServer");
    
    luaL_checktype(L, 2, LUA_TFUNCTION);

    // lua_pushvalue(L, 2); // copy function
    // udata->onRequestRef = luaL_ref(L, LUA_REGISTRYINDEX);

    lua_pushstring(L, "sex");
    return 1;
}

static int HttpServer_index(lua_State *L) {
    HttpServer* udata = (HttpServer*)luaL_checkudata(L, 1, "HttpServer"); // self
    const char* key = luaL_checkstring(L, 2); // key

    if (strcmp(key, "OnRequest") == 0) {
        lua_pushvalue(L, 1); // push user data to this closure
        lua_pushcclosure(L, HttpServer_OnRequestCallback, 1);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

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
        srv->~HttpServer();
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
        {"__index", HttpServer_index},
        {"__gc", HttpServer_gc}
    };
}
