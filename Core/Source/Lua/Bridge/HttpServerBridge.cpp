// === noobWarrior ===
// File: HttpServerBinding.cpp
// Started by: Hattozo
// Started on: 1/4/2026
// Description:
#include <NoobWarrior/Lua/Bridge/HttpServerBridge.h>
#include <NoobWarrior/HttpServer/Base/HttpServer.h>
#include <NoobWarrior/Lua/LuaState.h>

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

static int HttpServer_OnRequest(lua_State *L) {
    lua_pushnil(L);
    return 1;
}

static const luaL_Reg MetaFuncs[] = {
    {"__gc", HttpServer_gc},
    {NULL, NULL}
};

static const luaL_Reg ObjectFuncs[] = {
    {"new", HttpServer_new},
    {"OnRequest", HttpServer_OnRequest},
    {NULL, NULL}
};

HttpServerBridge::HttpServerBridge(LuaState* lua) : mLua(lua) {

}

void HttpServerBridge::Open() {
    lua_State *L = mLua->Get();

    luaL_newmetatable(L, "HttpServer");
    luaL_setfuncs(L, MetaFuncs, 0);

    lua_newtable(L);
    luaL_setfuncs(L, ObjectFuncs, 0);
    lua_setfield(L, -2, "__index");

    lua_pop(L, 1);

    lua_newtable(L);
    luaL_getmetatable(L, "HttpServer");
    lua_setmetatable(L, -2);
    lua_setglobal(L, "HttpServer");
}

