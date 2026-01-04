// === noobWarrior ===
// File: Plugin.cpp
// Started by: Hattozo
// Started on: 12/3/2025
// Description:
#include <NoobWarrior/Plugin.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/FileSystem/VirtualFileSystem.h>
#include <lua.h>

using namespace NoobWarrior;

/* NOTE: File names are relative to the path of the plugins folder in noobWarrior's user directory folder. */
Plugin::Plugin(const std::string &fileName, Core* core, bool includedInInstall) :
    mResponse(Response::Failed),
    mCore(core),
    mFileName(fileName),
    mVfs(nullptr),
    mIncludedInInstall(includedInInstall)
{
    std::filesystem::path fullDir = (!mIncludedInInstall ? mCore->GetUserDataDir() : mCore->GetInstallationDir()) / "plugins" / mFileName;

    // Use a virtual filesystem so that we can use both compressed archives and regular folders.
    VirtualFileSystem::Response fsRes = VirtualFileSystem::New(&mVfs, fullDir);

    if (fsRes != VirtualFileSystem::Response::Success || mVfs == nullptr) {
        mResponse = Response::Failed;
        return;
    }

    std::string pluginLuaString;

    mVfsHandle = mVfs->OpenHandle("/plugin.lua");
    std::string buf;
    while (mVfs->ReadHandleLine(mVfsHandle, &buf))
        pluginLuaString.append(buf + '\n');

    lua_State *L = mCore->GetLuaState()->Get();
    int res = luaL_dostring(L, pluginLuaString.c_str());
    if (res != LUA_OK) {
        Out("Plugin", "Failed to load plugin \"{}\" because plugin.lua failed with error: {}", mFileName, lua_tostring(L, -1));
        lua_pop(L, 1);
        mResponse = Response::Failed;
        return;
    }
    if (!lua_istable(L, -1)) {
        Out("Plugin", "Failed to load plugin \"{}\" because plugin.lua did not return a string.", mFileName);
        lua_pop(L, 1);
        mResponse = Response::Failed;
        return;
    }
    reference = luaL_ref(L, LUA_REGISTRYINDEX);
    mResponse = Response::Success;
}

Plugin::~Plugin() {
    if (Fail())
        return;
    if (mVfs != nullptr) {
        mVfs->CloseHandle(mVfsHandle);
        VirtualFileSystem::Free(mVfs);
    }
    luaL_unref(mCore->GetLuaState()->Get(), LUA_REGISTRYINDEX, reference);
}

Plugin::Response Plugin::Execute() {
    Out("Plugin", "Executing \"{}\"", mFileName);
    return Response::Success;
}

bool Plugin::Fail() {
    return mResponse != Response::Success;
}

Plugin::Response Plugin::GetInitResponse() {
    return mResponse;
}

std::string Plugin::GetFileName() {
    return mFileName;
}

const Plugin::Properties Plugin::GetProperties() {
    if (Fail()) {
        Out("Plugin", "GetProperties() called but plugin \"{}\" failed to initialize!", mFileName);
        return {};
    }
    lua_State *L = mCore->GetLuaState()->Get();
    PushLuaTable();

    lua_getfield(L, -1, "title");
    std::string title = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, -1, "version");
    std::string version = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, -1, "description");
    std::string description = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, -1, "critical");
    bool critical = lua_toboolean(L, -1);
    lua_pop(L, 1);

    Properties props {};
    props.FileName = mFileName;
    props.Title = title;
    props.Version = version;
    props.Description = description;
    props.IsCritical = critical;

    lua_pop(L, 1);
    return props;
}

void Plugin::PushLuaTable() {
    if (Fail())
        return;
    lua_rawgeti(mCore->GetLuaState()->Get(), LUA_REGISTRYINDEX, reference);
}