/*
 * Copyright (C) 2026 Hattozo
 *
 * This file is part of noobWarrior.
 *
 * noobWarrior is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * noobWarrior is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with noobWarrior; if not, see
 * <https://www.gnu.org/licenses/>.
 */
// === noobWarrior ===
// File: LuaState.cpp
// Started by: Hattozo
// Started on: 12/3/2025
// Description: Uses sol
#include <NoobWarrior/Lua/LuaState.h>
#include <NoobWarrior/Lua/LuaSignal.h>
#include <NoobWarrior/Lua/Bridge/PluginBridge.h>
#include <NoobWarrior/Lua/Lhp.h>
#include <NoobWarrior/Lua/Bridge/VfsBridge.h>
#include <NoobWarrior/Lua/Bridge/HttpServerBridge.h>
#include <NoobWarrior/Log.h>
#include <NoobWarrior/HttpServer/Base/HttpServer.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/FileSystem/VirtualFileSystem.h>
#include <NoobWarrior/FileSystem/OverlayFileSystem.h>
#include <NoobWarrior/FileSystem/StdFileSystem.h>
#include <NoobWarrior/FileSystem/ZipFileSystem.h>
#include <NoobWarrior/NoobWarrior.h>

#include <lua.hpp>
#include <sol/raii.hpp>
#include <sol/sol.hpp>

#include "files/global_env_metatable.lua.inc.cpp"
#include "files/rawget_path.lua.inc.cpp"
#include "files/serpent.lua.inc.cpp"
#include "files/json.lua.inc.cpp"

using namespace NoobWarrior;

static int printBS(lua_State *L) {
    int nargs = lua_gettop(L);

    std::string msg;
    for (int i = 1; i <= nargs; i++) {
        const char *str = lua_tolstring(L, i, NULL);
        msg += str;
        lua_pop(L, 1);
    }
    Out("Lua", msg);
    return 0;
}

LuaState::LuaState(Core* core) :
    mCore(core),
    mLhp(this),
    mLuaSignalBridge(this),
    mPluginBridge(this),
    mVfsBridge(this),
    mHttpServerBridge(this),
    mServerEmulatorBridge(this)
{
    open_libraries(
        sol::lib::base,
        sol::lib::package,
        sol::lib::coroutine,
        sol::lib::string,
        sol::lib::os,
        sol::lib::math,
        sol::lib::table,
        sol::lib::debug,
        sol::lib::bit32,
        sol::lib::io,
        sol::lib::ffi,
        sol::lib::jit
    );

    do_string(rawget_path_lua);
    do_string(global_env_metatable_lua);

    set("core", sol::make_light(mCore));
    set("print", printBS);
}

int LuaState::Open() {
    // lua_pushcfunction(mLuaState, printBS);
    // lua_setglobal(mLuaState, "error");

#define LOADLIBRARY(strVar, name) \
    sol::protected_function_result strVar##_res = do_string(strVar); \
    if (strVar##_res.valid() &&strVar##_res.get_type() == sol::type::table) { \
        set(name, strVar##_res.get<sol::table>()); \
    }
    
    LOADLIBRARY(serpent_lua, "serpent")
    LOADLIBRARY(json_lua, "json")
#undef LOADLIBRARY

    auto scriptType = new_usertype<LuaScript>("Script", sol::no_constructor);
    scriptType["new"] = [this](std::string src) {
        return std::make_unique<LuaScript>(this, this->globals(), src);
    };
    scriptType["GetUrl"] = &LuaScript::GetUrl;

    auto pluginType = new_usertype<Plugin>("Plugin", sol::no_constructor);
    pluginType["GetIdentifier"] = &Plugin::GetIdentifier;

    auto signalListenerType = new_usertype<LuaSignalListener>("SignalListener", sol::no_constructor);
    signalListenerType["Disconnect"] = &LuaSignalListener::Disconnect;

    auto signalType = new_usertype<LuaSignal>("Signal", sol::constructors<LuaSignal()>());
    signalType["Connect"] = &LuaSignal::Connect;
    signalType["Fire"] = &LuaSignal::LuaFire;

    auto vfsType = new_usertype<VirtualFileSystem>("VirtualFileSystem");
    vfsType["new"] = [](sol::this_state state, std::string path) {
        lua_State* L = state;
        luaL_error(L, "WIP");
    };
    vfsType["GetEntriesInDirectory"] = [this](VirtualFileSystem &vfs, const std::string &dir) -> sol::table {
        sol::table tbl = this->create_table();
        for (FSEntryInfo &entryInfo : vfs.GetEntriesInDirectory(dir)) {
            sol::table entryTbl = this->create_table();
            entryTbl["Failed"] = entryInfo.Failed;
            entryTbl["Exists"] = entryInfo.Exists;
            entryTbl["Type"] = entryInfo.Type == FSEntryInfo::Type::File ? "File" : "Directory";
            entryTbl["Size"] = entryInfo.Size;
            entryTbl["Name"] = entryInfo.Name;
            entryTbl["Path"] = entryInfo.Path;
            tbl.add(entryTbl);
        }
        return tbl;
    };
    vfsType["OpenHandle"] = &VirtualFileSystem::OpenHandle;
    vfsType["CloseHandle"] = &VirtualFileSystem::CloseHandle;
    vfsType["IsHandleEOF"] = &VirtualFileSystem::IsHandleEOF;
    vfsType["ReadHandleChunk"] = [](VirtualFileSystem &vfs, FSEntryHandle handle, int size) -> std::tuple<bool, std::string> {
        std::vector<unsigned char> buf;
        bool isReading = vfs.ReadHandleChunk(handle, &buf, size);
        return {isReading, std::string(buf.begin(), buf.end())};
    };
    vfsType["ReadHandleLine"] = [](VirtualFileSystem &vfs, FSEntryHandle handle) -> std::tuple<bool, std::string> {
        std::string buf;
        bool isReading = vfs.ReadHandleLine(handle, &buf);
        return {isReading, buf};
    };
    vfsType["EntryExists"] = &VirtualFileSystem::EntryExists;
    vfsType["DeleteEntry"] = &VirtualFileSystem::DeleteEntry;

    auto overlayFsType = new_usertype<OverlayFileSystem>("OverlayFileSystem", sol::constructors<OverlayFileSystem()>(), sol::base_classes, sol::bases<VirtualFileSystem>());
    auto stdFsType = new_usertype<StdFileSystem>("StdFileSystem", sol::constructors<StdFileSystem(const std::filesystem::path&)>(), sol::base_classes, sol::bases<VirtualFileSystem>());
    auto zipFsType = new_usertype<ZipFileSystem>("ZipFileSystem", sol::constructors<ZipFileSystem(const std::filesystem::path&)>(), sol::base_classes, sol::bases<VirtualFileSystem>());

    auto sqlDbType = new_usertype<SqlDb>("SqlDb", sol::constructors<SqlDb(), SqlDb(std::string, std::string)>());
    sqlDbType["ExecStatement"] = &SqlDb::ExecStatement;
    sqlDbType["SetPragma"] = &SqlDb::SetPragma;
    sqlDbType["Query"] = []() {

    };

    auto srvType = new_usertype<HttpServer>("HttpServer", sol::no_constructor);
    srvType["new"] = [this](std::string logName) {
        if (!logName.empty())
            return std::make_unique<HttpServer>(mCore, logName);
        else return std::make_unique<HttpServer>(mCore);
    };
    srvType["Start"] = &HttpServer::Start;
    srvType["Stop"] = &HttpServer::Stop;
    srvType["GetVfs"] = &HttpServer::GetVfs;
    srvType["MountVolume"] = [](sol::this_state state, HttpServer& self, std::string root, std::string realPath) -> void {
        lua_State* L = state;
        VirtualFileSystem::Response res = self.MountVolume(root, Url(realPath));
        if (res != VirtualFileSystem::Response::Success) {
            luaL_error(L, "failed to mount volume");
        }
    };
    srvType["UnmountVolume"] = [](sol::this_state state, HttpServer& self, std::string root, std::string realPath) -> void {
        lua_State* L = state;
        VirtualFileSystem::Response res = self.UnmountVolume(root, Url(realPath));
        if (res != VirtualFileSystem::Response::Success) {
            luaL_error(L, "failed to unmount volume");
        }
    };
    srvType["OnRequest"] = sol::property([](HttpServer &srv) {
        return srv.GetOnRequestSignal();
    });

    auto emuType = new_usertype<ServerEmulator>("ServerEmulator");
    set("emu", mCore->GetServerEmulator());

    sol::table lhpLib = create_table();
    lhpLib.set_function("Render", [this](sol::this_state state, sol::this_environment thisEnv, std::string input) -> std::string {
        lua_State* L = state;
        sol::environment& env = thisEnv;

        std::string output;
        Lhp::RenderResponse res = mLhp.Render(env, input, &output);
        if (res != Lhp::RenderResponse::Success) {
            luaL_error(L, "failed to render page using lhp");
        }
        return output;
    });
    lhpLib.set_function("RenderFile", [this](sol::this_state state, sol::this_environment thisEnv, std::string fileLocation) -> std::string {
        lua_State* L = state;
        sol::environment& env = thisEnv;
        
        // In each LuaScript (yes we create objects for each script that autoruns) we include a "script" variable
        // in their personalized environment that contains a self-reference to the script that is currently being ran.
        // Basically it's like Roblox's "script" global.
        sol::userdata data = env["script"];
        LuaScript& script = data.as<LuaScript>();

        UrlContext ctx {};
        ctx.DefaultProtocolType = script.GetUrl().GetProtocol();
        ctx.DefaultHostName = script.GetUrl().GetHostName();
        ctx.Cwd = script.GetUrl().GetCwd();

        Url url(fileLocation, ctx);

        std::string output;
        Lhp::RenderResponse res = mLhp.Render(env, url, &output);
        if (res != Lhp::RenderResponse::Success) {
            luaL_error(L, "failed to render page using lhp");
        }
        return output;
    });
    set("lhp", lhpLib);

    Out("Lua", "Initialized Lua");
    return 1;
}

bool LuaState::Opened() {
    return true;
}

lua_State* LuaState::Get() {
    return lua_state();
}

Lhp *LuaState::GetLhp() {
    return &mLhp;
}

Core *LuaState::GetCore() {
    return mCore;
}
