// === noobWarrior ===
// File: VfsBinding.cpp
// Started by: Hattozo
// Started on: 1/10/2026
// Description:
#include <NoobWarrior/Lua/VfsBinding.h>
#include <NoobWarrior/Lua/LuaState.h>
#include <NoobWarrior/FileSystem/VirtualFileSystem.h>
#include <NoobWarrior/FileSystem/StdFileSystem.h>
#include <NoobWarrior/FileSystem/ZipFileSystem.h>

using namespace NoobWarrior;

static int VirtualFileSystem_new(lua_State *L) {
    const char* pathStr = luaL_checkstring(L, 1);
    VirtualFileSystem* vfs = nullptr;

    switch (VirtualFileSystem::GetFormatFromPath(pathStr)) {
    default:
        return luaL_error(L, "invalid format for virtual filesystem");
    case VirtualFileSystem::Format::Standard:
        vfs = (StdFileSystem*)lua_newuserdata(L, sizeof(StdFileSystem));
        new(vfs) StdFileSystem(pathStr);
        break;
    case VirtualFileSystem::Format::Zip:
        vfs = (ZipFileSystem*)lua_newuserdata(L, sizeof(ZipFileSystem));
        new(vfs) ZipFileSystem(pathStr);
        break;
    }
    return 1;
}

static int VirtualFileSystem_gc(lua_State *L) {
    VirtualFileSystem* vfs = (VirtualFileSystem*)luaL_checkudata(L, 1, "VirtualFileSystem");
    if (vfs != nullptr) {
        delete vfs;
    }
    return 0;
}

static const luaL_Reg VfsFuncs[] = {
    {"new", VirtualFileSystem_new},
    {nullptr, nullptr}
};

static const luaL_Reg VfsMetaFuncs[] = {
    {"__gc", VirtualFileSystem_gc},
    {NULL, NULL}
};

VfsBinding::VfsBinding(LuaState* lua) : mLua(lua) {}

void VfsBinding::Open() {
    lua_State *L = mLua->Get();
    luaL_newmetatable(L, "VirtualFileSystem");

    luaL_newlib(L, VfsFuncs);
    lua_setglobal(L, "VirtualFileSystem");
}

void VfsBinding::Close() {
    lua_State *L = mLua->Get();
    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, "VirtualFileSystem");
}