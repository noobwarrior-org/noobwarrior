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
#include <NoobWarrior/Lua/Lhp.h>
#include <NoobWarrior/Log.h>
#include <NoobWarrior/Registry.h>
#include <NoobWarrior/HttpServer/Base/HttpServer.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/FileSystem/VirtualFileSystem.h>
#include <NoobWarrior/FileSystem/OverlayFileSystem.h>
#include <NoobWarrior/FileSystem/StdFileSystem.h>
#include <NoobWarrior/FileSystem/ZipFileSystem.h>
#include <NoobWarrior/NoobWarrior.h>

#include <lua.hpp>
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

int exception_handler(lua_State* L, sol::optional<const std::exception&> maybe_exception, sol::string_view description) {
    Out("Lua", "Exception occurred: {}", description);
    return sol::stack::push(L, description);
}

LuaState::LuaState(Core* core) :
    mCore(core),
    mLhp(this)
{
    set_exception_handler(&exception_handler);

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
    if (strVar##_res.valid() && strVar##_res.get_type() == sol::type::table) { \
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

    sol::table regLib = create_table();
    regLib["SetKeyValue"] = [this](sol::this_state state, std::string key, sol::object value) {
        lua_State* L = state;
        Registry* reg = mCore->GetRegistry();
        if (key.empty()) {
            luaL_error(L, "error setting value for key \"%s\": key is empty", key.c_str());
            return;
        }
        if (key.starts_with('.')) {
            luaL_error(L, "error setting value for key \"%s\": key cannot start with a period", key.c_str());
            return;
        }
        if (key.ends_with('.')) {
            luaL_error(L, "error setting value for key \"%s\": key cannot end with a period", key.c_str());
            return;
        }
        (*this)["__REG_BUF"] = value;
        sol::protected_function_result res = safe_script(std::format("{}.{} = __REG_BUF", reg->GetGlobalName(), key));
        if (!res.valid()) {
            luaL_error(L, "error setting value for key \"%s\": failed to run script", key.c_str());
            return;
        }
        (*this)["__REG_BUF"] = sol::lua_nil;
    };
    regLib["GetKeyValue"] = [this](sol::this_state state, std::string key) -> sol::object {
        lua_State* L = state;
        Registry* reg = mCore->GetRegistry();
        if (key.empty()) {
            luaL_error(L, "error getting value for key \"%s\": key is empty", key.c_str());
            return sol::lua_nil;
        }
        if (key.starts_with('.')) {
            luaL_error(L, "error getting value for key \"%s\": key cannot start with a period", key.c_str());
            return sol::lua_nil;
        }
        if (key.ends_with('.')) {
            luaL_error(L, "error getting value for key \"%s\": key cannot end with a period", key.c_str());
            return sol::lua_nil;
        }
        sol::protected_function_result res = safe_script(std::format("return {}.{}", reg->GetGlobalName(), key), sol::script_pass_on_error);
        if (!res.valid()) {
            luaL_error(L, "error getting value for key \"%s\": failed to run script", key.c_str());
            return sol::lua_nil;
        }
        sol::object obj = res.get<sol::object>();
        return obj;
    };
    regLib["SetKeyValueIfNotSet"] = [this](sol::this_state state, std::string key, sol::object value) {
        lua_State* L = state;
        Registry* reg = mCore->GetRegistry();
        if (key.empty()) {
            luaL_error(L, "error setting value for key \"%s\": key is empty", key.c_str());
            return;
        }
        if (key.starts_with('.')) {
            luaL_error(L, "error setting value for key \"%s\": key cannot start with a period", key.c_str());
            return;
        }
        if (key.ends_with('.')) {
            luaL_error(L, "error setting value for key \"%s\": key cannot end with a period", key.c_str());
            return;
        }
        sol::protected_function_result res = safe_script(std::format("return {}.{}", reg->GetGlobalName(), key), sol::script_pass_on_error);
        if (!res.valid()) {
            luaL_error(L, "error setting value for key \"%s\": failed to run script", key.c_str());
            return;
        }
        if (res.get_type() != sol::type::lua_nil) {
            return;
        }
        (*this)["__REG_BUF"] = value;
        sol::protected_function_result res2 = safe_script(std::format("{}.{} = __REG_BUF", reg->GetGlobalName(), key));
        if (!res2.valid()) {
            luaL_error(L, "error setting value for key \"%s\": failed to run script", key.c_str());
            return;
        }
        (*this)["__REG_BUF"] = sol::lua_nil;
    };
    set("reg", regLib);

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

    auto sqlDbType = new_usertype<SqlDb>("SqlDb", sol::constructors<SqlDb(), SqlDb(const std::string&, const std::string&)>());
    sqlDbType["ExecStatement"] = &SqlDb::ExecStatement;
    sqlDbType["SetPragma"] = &SqlDb::SetPragma;

    // This method works similarly to https://wiki.facepunch.com/gmod/sql.Query
    sqlDbType["Query"] = [this](SqlDb &db, const std::string &stmtStr) -> sol::object {
        bool failed = false;
        SqlRows rows = db.Query(stmtStr, &failed);
        if (failed)
            return sol::make_object(*this, false);
        if (rows.empty())
            return sol::lua_nil;
        sol::table rowsTbl = create_table();
        for (SqlRow& row : rows) {
            sol::table rowTbl = create_table();
            for (SqlColumn& col : row) {
                rowTbl.set(col.first, col.second);
            }
            rowsTbl.add(rowTbl);
        }
        return rowsTbl;
    };

    // This method also works similarly to https://wiki.facepunch.com/gmod/sql.QueryTyped
    sqlDbType["QueryTyped"] = [this](SqlDb &db, const std::string &stmtStr, sol::variadic_args va) -> sol::object {
        SqlRows rows;
        Statement stmt = db.PrepareStatement(stmtStr);
        if (stmt.Fail())
            return sol::make_object(*this, false);

        int idx = 0;
        for (auto v : va) {
            idx++;
            stmt.Bind(idx, v.as<SqlValue>());
        }
        while (stmt.Step() == SQLITE_ROW) {
            rows.push_back(stmt.GetColumns());
        }
        if (rows.empty())
            return sol::lua_nil;

        sol::table rowsTbl = create_table();
        for (SqlRow& row : rows) {
            sol::table rowTbl = create_table();
            for (SqlColumn& col : row) {
                rowTbl.set(col.first, col.second);
            }
            rowsTbl.add(rowTbl);
        }
        return rowsTbl;
    };

    auto emuDbType = new_usertype<EmuDb>("EmuDb", sol::constructors<EmuDb(), EmuDb(const std::string&, bool)>(), sol::base_classes, sol::bases<SqlDb>());

    auto emuDbMgrType = new_usertype<EmuDbManager>("EmuDbManager", sol::no_constructor);
    emuDbMgrType["GetMasterDatabase"] = &EmuDbManager::GetMasterDatabase;
    emuDbMgrType["GetMountedDatabases"] = &EmuDbManager::GetMountedDatabases;

    auto srvType = new_usertype<HttpServer>("HttpServer", sol::no_constructor);
    srvType["new"] = [this](std::string logName) {
        if (!logName.empty())
            return std::make_unique<HttpServer>(mCore, logName);
        else return std::make_unique<HttpServer>(mCore);
    };
    srvType["Start"] = &HttpServer::Start;
    srvType["Stop"] = &HttpServer::Stop;
    srvType["GetVfs"] = &HttpServer::GetVfs;
    srvType["MountVolume"] = [](sol::this_state state, sol::this_environment tenv, HttpServer& self, std::string root, std::string realPath) -> void {
        lua_State* L = state;
        sol::environment env = tenv;

        UrlContext ctx {};
        LuaScript* callingScript = env["script"].get_or<LuaScript*>(nullptr);
        if (callingScript != nullptr) {
            Url callingScriptUrl = callingScript->GetUrl();
            ctx = {
                .Cwd = callingScriptUrl.GetDirectory(),
                .DefaultProtocolType = callingScriptUrl.GetProtocol(),
                .DefaultHostName = callingScriptUrl.GetHostName(),
            };
        }

        VirtualFileSystem::Response res = self.MountVolume(root, Url(realPath, ctx));
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
    srvType["PreStart"] = sol::property([](HttpServer &srv) {
        return srv.GetPreStartSignal();
    });
    srvType["PreStop"] = sol::property([](HttpServer &srv) {
        return srv.GetPreStopSignal();
    });
    srvType["PostStart"] = sol::property([](HttpServer &srv) {
        return srv.GetPostStartSignal();
    });
    srvType["PostStop"] = sol::property([](HttpServer &srv) {
        return srv.GetPostStopSignal();
    });
    srvType["OnRequest"] = sol::property([](HttpServer &srv) {
        return srv.GetOnRequestSignal();
    });

    auto srvEmuType = new_usertype<ServerEmulator>("ServerEmulator", sol::no_constructor, sol::base_classes, sol::bases<HttpServer>());
    srvEmuType["GetGameServers"] = [this](ServerEmulator &emu) {
        std::vector<EngineStartParameters> params = emu.GetGameServers();
        sol::table tbl = create_table();
        return tbl;
    };

    sol::table lhpLib = create_table();
    lhpLib.set_function("Render", [this](sol::this_state state, sol::this_environment thisEnv, std::string input, const std::optional<sol::table> &globalsList) -> std::string {
        lua_State* L = state;
        sol::environment& env = thisEnv;
        sol::environment globalsEnv = sol::environment(*this, sol::create, env);
        if (globalsList.has_value()) {
            for (const auto &kv : globalsList.value()) {
                globalsEnv[kv.first] = kv.second;
            }
        }

        std::string output;
        Lhp::RenderResponse res = mLhp.Render(globalsEnv, input, &output);
        if (res != Lhp::RenderResponse::Success) {
            luaL_error(L, "failed to render page using lhp");
        }
        return output;
    });
    lhpLib.set_function("RenderFile", [this](sol::this_state state, sol::this_environment thisEnv, std::string fileLocation, const std::optional<sol::table> &globalsList) -> std::string {
        lua_State* L = state;
        sol::environment& env = thisEnv;
        sol::environment globalsEnv = sol::environment(*this, sol::create, env);
        if (globalsList.has_value()) {
            for (const auto &kv : globalsList.value()) {
                globalsEnv[kv.first] = kv.second;
            }
        }
        
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
        Lhp::RenderResponse res = mLhp.Render(globalsEnv, url, &output);
        if (res != Lhp::RenderResponse::Success) {
            luaL_error(L, "failed to render page using lhp");
        }
        return output;
    });
    set("lhp", lhpLib);

    sol::table coreLib = create_table();
    coreLib.set_function("GetVersion", []() {
        return NOOBWARRIOR_VERSION;
    });
    set("core", coreLib);

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

bool LuaState::IsScriptLoading(const std::string &resolvedUrl) {
    return mLoadingScripts.contains(resolvedUrl);
}

void LuaState::MarkScriptLoading(const std::string &resolvedUrl) {
    mLoadingScripts.insert(resolvedUrl);
}

void LuaState::UnmarkScriptLoading(const std::string &resolvedUrl) {
    mLoadingScripts.erase(resolvedUrl);
}

void LuaState::CacheModule(const std::string& resolvedUrl, std::unique_ptr<LuaScript> module) {
    mCachedModules[resolvedUrl] = std::move(module);
}

LuaScript* LuaState::RetrieveCachedModuleFromResolvedUrl(const std::string &resolvedUrl) const {
    if (!mCachedModules.contains(resolvedUrl))
        return nullptr;
    return mCachedModules.at(resolvedUrl).get();
}
