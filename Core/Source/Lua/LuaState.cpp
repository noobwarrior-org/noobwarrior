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
#include <cpr/cpr.h>

#include <NoobWarrior/Lua/LuaState.h>
#include <NoobWarrior/Lua/LuaSignal.h>
#include <NoobWarrior/Lua/Lhp.h>
#include <NoobWarrior/Log.h>
#include <NoobWarrior/Registry.h>
#include <NoobWarrior/HttpServer/Base/HttpServer.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/HttpServer/Emulator/AvatarAppearance.h>
#include <NoobWarrior/FileSystem/VirtualFileSystem.h>
#include <NoobWarrior/FileSystem/OverlayFileSystem.h>
#include <NoobWarrior/FileSystem/StdFileSystem.h>
#include <NoobWarrior/FileSystem/ZipFileSystem.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Console/Command/Command.h>
#include <NoobWarrior/Console/Command/FuncCommand.h>

#include <lua.hpp>
#include <sol/sol.hpp>

#include <curl/curl.h>

#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/kdf.h>
#include <openssl/params.h>
#include <openssl/core_names.h>
#include <openssl/crypto.h>

#include <charconv>

#include "NoobWarrior/Url.h"
#include "files/global_env_metatable.lua.inc.cpp"
#include "files/rawget_path.lua.inc.cpp"
#include "files/serpent.lua.inc.cpp"
#include "files/json.lua.inc.cpp"

using namespace NoobWarrior;

static std::string HexEncode(const unsigned char *data, size_t len) {
    std::string out;
    out.reserve(len * 2);
    for (size_t i = 0; i < len; i++)
        out += std::format("{:02x}", data[i]);
    return out;
}

static bool HexDecode(const std::string &hex, std::vector<unsigned char> &out) {
    if (hex.size() % 2 != 0) return false;
    out.clear();
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        unsigned int byte = 0;
        auto [ptr, ec] = std::from_chars(hex.data() + i, hex.data() + i + 2, byte, 16);
        if (ec != std::errc() || ptr != hex.data() + i + 2) return false;
        out.push_back(static_cast<unsigned char>(byte));
    }
    return true;
}

static bool Argon2idHash(const std::string &password, const unsigned char *salt, size_t saltLen,
                          unsigned char *out, size_t outLen) {
    uint32_t t_cost = 2, m_cost = 1u << 16, lanes = 1, threads = 1;
    EVP_KDF *kdf = EVP_KDF_fetch(nullptr, "ARGON2ID", nullptr);
    EVP_KDF_CTX *kctx = kdf ? EVP_KDF_CTX_new(kdf) : nullptr;
    EVP_KDF_free(kdf);
    OSSL_PARAM params[] = {
        OSSL_PARAM_octet_string(OSSL_KDF_PARAM_PASSWORD, const_cast<char*>(password.data()), password.size()),
        OSSL_PARAM_octet_string(OSSL_KDF_PARAM_SALT, const_cast<unsigned char*>(salt), saltLen),
        OSSL_PARAM_uint32(OSSL_KDF_PARAM_ITER, &t_cost),
        OSSL_PARAM_uint32(OSSL_KDF_PARAM_ARGON2_MEMCOST, &m_cost),
        OSSL_PARAM_uint32(OSSL_KDF_PARAM_ARGON2_LANES, &lanes),
        OSSL_PARAM_uint32(OSSL_KDF_PARAM_THREADS, &threads),
        OSSL_PARAM_END
    };
    bool ok = kctx && EVP_KDF_derive(kctx, out, outLen, params) > 0;
    EVP_KDF_CTX_free(kctx);
    return ok;
}

struct LuaNetClient {
    std::vector<std::pair<std::string, std::string>> defaultHeaders;
    long timeout = 30;
};

static int printBS(lua_State *L) {
    int nargs = lua_gettop(L);

    std::string msg;
    for (int i = 1; i <= nargs; i++) {
        size_t len = 0;
        const char *str = luaL_tolstring(L, i, &len);
        if (i > 1) msg += '\t';
        msg.append(str, len);
        lua_pop(L, 1);  // pop the string pushed by luaL_tolstring
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

    // First we start with enums
    new_enum("ItemType",
        "Asset", ItemType::Asset,
        "Badge", ItemType::Badge,
        "Bundle", ItemType::Bundle,
        "DevProduct", ItemType::DevProduct,
        "Group", ItemType::Group,
        "Outfit", ItemType::Outfit,
        "Pass", ItemType::Pass,
        "Set", ItemType::Set,
        "Universe", ItemType::Universe,
        "User", ItemType::User
    );

    new_enum("ProtocolType",
        "Unsupported", ProtocolType::Unsupported,
        "File", ProtocolType::File,
        "Http", ProtocolType::Http,
        "Https", ProtocolType::Https,
        "InstallData", ProtocolType::InstallData,
        "UserData", ProtocolType::UserData,
        "Database", ProtocolType::Database,
        "Plugin", ProtocolType::Plugin,
        "PluginData", ProtocolType::PluginData,
        "RbxAssetId", ProtocolType::RbxAssetId,
        "RbxThumb", ProtocolType::RbxThumb
    );

    new_enum("SqlResponse",
        "Failed", SqlDb::Response::Failed,
        "Success", SqlDb::Response::Success,
        "CantOpen", SqlDb::Response::CantOpen,
        "DidNothing", SqlDb::Response::DidNothing,
        "DatabaseFailed", SqlDb::Response::DatabaseFailed,
        "ConstraintViolation", SqlDb::Response::ConstraintViolation,
        "Busy", SqlDb::Response::Busy,
        "Misuse", SqlDb::Response::Misuse,
        "NotFound", SqlDb::Response::NotFound,
        "BlobTooLarge", SqlDb::Response::BlobTooLarge,
        "MissingBlob", SqlDb::Response::MissingBlob,
        "BlobOpenFailed", SqlDb::Response::BlobOpenFailed,
        "BlobCompressionFailed", SqlDb::Response::BlobCompressionFailed,
        "BlobDecompressionFailed", SqlDb::Response::BlobDecompressionFailed
    );

    new_enum("CompressionType",
        "None", EmuDb::CompressionType::None,
        "ZStandard", EmuDb::CompressionType::ZStandard
    );

    new_enum("AssetType",
        "None", Roblox::AssetType::None,
        "Image", Roblox::AssetType::Image,
        "TShirt", Roblox::AssetType::TShirt,
        "Audio", Roblox::AssetType::Audio,
        "Mesh", Roblox::AssetType::Mesh,
        "Lua", Roblox::AssetType::Lua,
        "Hat", Roblox::AssetType::Hat,
        "Place", Roblox::AssetType::Place,
        "Model", Roblox::AssetType::Model,
        "Shirt", Roblox::AssetType::Shirt,
        "Pants", Roblox::AssetType::Pants,
        "Decal", Roblox::AssetType::Decal,
        "Head", Roblox::AssetType::Head,
        "Face", Roblox::AssetType::Face,
        "Gear", Roblox::AssetType::Gear,
        "Badge", Roblox::AssetType::Badge,
        "Animation", Roblox::AssetType::Animation,
        "Torso", Roblox::AssetType::Torso,
        "RightArm", Roblox::AssetType::RightArm,
        "LeftArm", Roblox::AssetType::LeftArm,
        "LeftLeg", Roblox::AssetType::LeftLeg,
        "RightLeg", Roblox::AssetType::RightLeg,
        "Package", Roblox::AssetType::Package,
        "Gamepass", Roblox::AssetType::Gamepass,
        "Plugin", Roblox::AssetType::Plugin,
        "MeshPart", Roblox::AssetType::MeshPart,
        "HairAccessory", Roblox::AssetType::HairAccessory,
        "FaceAccessory", Roblox::AssetType::FaceAccessory,
        "NeckAccessory", Roblox::AssetType::NeckAccessory,
        "ShoulderAccessory", Roblox::AssetType::ShoulderAccessory,
        "FrontAccessory", Roblox::AssetType::FrontAccessory,
        "BackAccessory", Roblox::AssetType::BackAccessory,
        "WaistAccessory", Roblox::AssetType::WaistAccessory,
        "ClimbAnimation", Roblox::AssetType::ClimbAnimation,
        "DeathAnimation", Roblox::AssetType::DeathAnimation,
        "FallAnimation", Roblox::AssetType::FallAnimation,
        "IdleAnimation", Roblox::AssetType::IdleAnimation,
        "JumpAnimation", Roblox::AssetType::JumpAnimation,
        "RunAnimation", Roblox::AssetType::RunAnimation,
        "SwimAnimation", Roblox::AssetType::SwimAnimation,
        "WalkAnimation", Roblox::AssetType::WalkAnimation,
        "PoseAnimation", Roblox::AssetType::PoseAnimation,
        "EarAccessory", Roblox::AssetType::EarAccessory,
        "EyeAccessory", Roblox::AssetType::EyeAccessory,
        "EmoteAnimation", Roblox::AssetType::EmoteAnimation,
        "Video", Roblox::AssetType::Video,
        "TShirtAccessory", Roblox::AssetType::TShirtAccessory,
        "ShirtAccessory", Roblox::AssetType::ShirtAccessory,
        "PantsAccessory", Roblox::AssetType::PantsAccessory,
        "JacketAccessory", Roblox::AssetType::JacketAccessory,
        "SweaterAccessory", Roblox::AssetType::SweaterAccessory,
        "ShortsAccessory", Roblox::AssetType::ShortsAccessory,
        "LeftShoeAccessory", Roblox::AssetType::LeftShoeAccessory,
        "RightShoeAccessory", Roblox::AssetType::RightShoeAccessory,
        "DressSkirtAccessory", Roblox::AssetType::DressSkirtAccessory,
        "FontFamily", Roblox::AssetType::FontFamily,
        "EyebrowAccessory", Roblox::AssetType::EyebrowAccessory,
        "EyelashAccessory", Roblox::AssetType::EyelashAccessory,
        "MoodAnimation", Roblox::AssetType::MoodAnimation,
        "DynamicHead", Roblox::AssetType::DynamicHead,
        "FaceMakeup", Roblox::AssetType::FaceMakeup,
        "LipMakeup", Roblox::AssetType::LipMakeup,
        "EyeMakeup", Roblox::AssetType::EyeMakeup
    );

    new_enum("CreatorType",
        "User", Roblox::CreatorType::User,
        "Group", Roblox::CreatorType::Group
    );

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
        sol::protected_function_result res = safe_script(std::format("return rawget_path({}, '{}')", reg->GetGlobalName(), key), sol::script_pass_on_error);
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
    regLib["SetKeyComment"] = [this](std::string key, std::string comment) {
        Registry* reg = mCore->GetRegistry();
        reg->SetKeyComment(key.c_str(), comment.c_str());
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
    vfsType["WriteFile"] = [](VirtualFileSystem &vfs, const std::string &path, const std::string &data) -> bool {
        std::vector<unsigned char> bytes(data.begin(), data.end());
        return vfs.WriteFile(path, bytes) == VirtualFileSystem::Response::Success;
    };
    vfsType["CreateDirectories"] = [](VirtualFileSystem &vfs, const std::string &path) -> bool {
        return vfs.CreateDirectories(path) == VirtualFileSystem::Response::Success;
    };

    auto overlayFsType = new_usertype<OverlayFileSystem>("OverlayFileSystem", sol::constructors<OverlayFileSystem()>(), sol::base_classes, sol::bases<VirtualFileSystem>());
    auto stdFsType = new_usertype<StdFileSystem>("StdFileSystem", sol::constructors<StdFileSystem(const std::filesystem::path&)>(), sol::base_classes, sol::bases<VirtualFileSystem>());
    auto zipFsType = new_usertype<ZipFileSystem>("ZipFileSystem", sol::constructors<ZipFileSystem(const std::filesystem::path&)>(), sol::base_classes, sol::bases<VirtualFileSystem>());

    auto sqlDbType = new_usertype<SqlDb>("SqlDb", sol::no_constructor);
    sqlDbType["new"] = [this](std::string url, std::string logName) {
        std::filesystem::path path = Url(url).ResolveAsLocalPath(mCore);
        return std::make_unique<SqlDb>(path.string(), !logName.empty() ? logName : "SqlDb");
    };
    sqlDbType["ExecStatement"] = &SqlDb::ExecStatement;
    sqlDbType["SetPragma"] = &SqlDb::SetPragma;
    sqlDbType["GetFileName"] = &SqlDb::GetFileName;
    sqlDbType["GetFilePath"] = [](SqlDb &db) -> std::string { return db.GetFilePath().string(); };

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
            sol::object o = v;
            if (o.get_type() == sol::type::number) {
                // LuaJIT numbers are doubles; bind whole numbers as 64-bit ints so that
                // ids above 2^31 (User.Id, asset ids, ...) aren't truncated to int32.
                double d = o.as<double>();
                int64_t i = static_cast<int64_t>(d);
                if (static_cast<double>(i) == d) stmt.Bind(idx, i);
                else stmt.Bind(idx, d);
            } else {
                stmt.Bind(idx, o.as<SqlValue>());
            }
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
                // Don't put this value in if it's null (std::monostate is supposed to represent a null value btw)
                if (!std::holds_alternative<std::monostate>(col.second)) {
                    rowTbl.set(col.first, col.second);
                }
            }
            rowsTbl.add(rowTbl);
        }
        return rowsTbl;
    };
    
    auto bytesToString = [](const std::vector<unsigned char> &bytes) -> std::string {
        return std::string(bytes.begin(), bytes.end());
    };
    auto stringToBytes = [](const std::string &str) -> std::vector<unsigned char> {
        return std::vector<unsigned char>(str.begin(), str.end());
    };
    auto tableToSqlRow = [](const sol::table &tbl) -> SqlRow {
        SqlRow row;
        for (const auto &kv : tbl) {
            if (kv.first.get_type() != sol::type::string)
                continue;
            const sol::object &o = kv.second;
            SqlValue val;
            if (o.get_type() == sol::type::number) {
                // Whole numbers -> int64 so large ids aren't truncated to int32.
                double d = o.as<double>();
                int64_t i = static_cast<int64_t>(d);
                val = (static_cast<double>(i) == d) ? SqlValue(i) : SqlValue(d);
            } else {
                val = o.as<SqlValue>();
            }
            row.push_back({kv.first.as<std::string>(), val});
        }
        return row;
    };
    auto assetSummaryToTable = [this](const EmuDb::AssetSummary &s) -> sol::table {
        sol::table t = create_table();
        t["Id"] = s.Id;
        t["Name"] = s.Name;
        t["Description"] = s.Description;
        t["Type"] = s.Type;
        if (s.UserId.has_value()) t["UserId"] = s.UserId.value();
        if (s.GroupId.has_value()) t["GroupId"] = s.GroupId.value();
        t["Created"] = s.Created;
        t["Updated"] = s.Updated;
        return t;
    };

    auto emuDbType = new_usertype<EmuDb>("EmuDb", sol::no_constructor, sol::base_classes, sol::bases<SqlDb>());
    emuDbType["new"] = [this](std::string url, sol::variadic_args va) {
        std::filesystem::path path = Url(url).ResolveAsLocalPath(mCore);
        return std::make_unique<EmuDb>(path.string(), va.size() > 0 ? va.get<bool>(0) : false);
    };

    emuDbType["IsZstdCompressed"] = [stringToBytes](const std::string &data) -> bool {
        return EmuDb::IsZstdCompressed(stringToBytes(data));
    };
    emuDbType["RetrieveAssetTypeImageData"] = [bytesToString](Roblox::AssetType type) -> std::string {
        return bytesToString(EmuDb::RetrieveAssetTypeImageData(type));
    };

    emuDbType["GetMigrationVersion"] = &EmuDb::GetMigrationVersion;
    emuDbType["GetMigrationFailMsg"] = &EmuDb::GetMigrationFailMsg;
    emuDbType["SaveAs"] = [this](EmuDb &db, std::string url) {
        return db.SaveAs(Url(url).ResolveAsLocalPath(mCore).string());
    };
    emuDbType["WriteChangesToDisk"] = &EmuDb::WriteChangesToDisk;
    emuDbType["IsDirty"] = &EmuDb::IsDirty;
    emuDbType["MarkDirty"] = &EmuDb::MarkDirty;
    emuDbType["UnmarkDirty"] = &EmuDb::UnmarkDirty;

    /* Meta */
    emuDbType["GetMetaKeyValue"] = &EmuDb::GetMetaKeyValue;
    emuDbType["GetTitle"] = &EmuDb::GetTitle;
    emuDbType["GetDescription"] = &EmuDb::GetDescription;
    emuDbType["GetVersion"] = &EmuDb::GetVersion;
    emuDbType["GetAuthor"] = &EmuDb::GetAuthor;
    emuDbType["GetIcon"] = [bytesToString](EmuDb &db) -> std::string {
        return bytesToString(db.GetIcon());
    };
    emuDbType["GetCompressionType"] = &EmuDb::GetCompressionType;
    emuDbType["SetMetaKeyValue"] = &EmuDb::SetMetaKeyValue;
    emuDbType["SetTitle"] = &EmuDb::SetTitle;
    emuDbType["SetDescription"] = &EmuDb::SetDescription;
    emuDbType["SetVersion"] = &EmuDb::SetVersion;
    emuDbType["SetAuthor"] = &EmuDb::SetAuthor;
    emuDbType["SetIcon"] = [stringToBytes](EmuDb &db, std::string icon) {
        return db.SetIcon(stringToBytes(icon));
    };

    /* Generic items */
    emuDbType["AddBlob"] = [stringToBytes](EmuDb &db, std::string data) -> std::tuple<SqlDb::Response, std::string> {
        std::string hash;
        SqlDb::Response res = db.AddBlob(stringToBytes(data), &hash);
        return {res, hash};
    };
    emuDbType["AddItem"] = [tableToSqlRow](EmuDb &db, ItemType type, sol::table row) {
        return db.AddItem(type, tableToSqlRow(row));
    };
    emuDbType["UpdateItem"] = [tableToSqlRow](EmuDb &db, ItemType type, int64_t id, sol::table row) {
        return db.UpdateItem(type, id, tableToSqlRow(row));
    };
    emuDbType["DeleteItem"] = &EmuDb::DeleteItem;
    emuDbType["DoesItemExist"] = &EmuDb::DoesItemExist;

    /* Asset data */
    emuDbType["AttachDataToAsset"] = [stringToBytes](EmuDb &db, int64_t id, int version, std::string data) {
        return db.AttachDataToAsset(id, version, stringToBytes(data));
    };
    emuDbType["DetachDataFromAsset"] = &EmuDb::DetachDataFromAsset;
    emuDbType["AttachBlobHashToAsset"] = &EmuDb::AttachBlobHashToAsset;
    emuDbType["DetachBlobHashFromAsset"] = &EmuDb::DetachBlobHashFromAsset;
    emuDbType["AttachThumbnailDataToAsset"] = [stringToBytes](EmuDb &db, int64_t id, std::string data) {
        return db.AttachThumbnailDataToAsset(id, stringToBytes(data));
    };
    emuDbType["AttachHistoricalDataToAsset"] = [tableToSqlRow](EmuDb &db, int64_t id, sol::table row) {
        return db.AttachHistoricalDataToAsset(id, tableToSqlRow(row));
    };
    emuDbType["DetachHistoricalDataFromAsset"] = [tableToSqlRow](EmuDb &db, int64_t id, sol::table row) {
        return db.DetachHistoricalDataFromAsset(id, tableToSqlRow(row));
    };
    emuDbType["AttachMicrotransactionDataToAsset"] = [tableToSqlRow](EmuDb &db, int64_t id, sol::table row) {
        return db.AttachMicrotransactionDataToAsset(id, tableToSqlRow(row));
    };
    emuDbType["DetachMicrotransactionDataFromAsset"] = [tableToSqlRow](EmuDb &db, int64_t id, sol::table row) {
        return db.DetachMicrotransactionDataFromAsset(id, tableToSqlRow(row));
    };
    emuDbType["AddThumbnailToPlace"] = &EmuDb::AddThumbnailToPlace;
    emuDbType["RemoveThumbnailFromPlace"] = &EmuDb::RemoveThumbnailFromPlace;
    emuDbType["RenderThumbnailForAsset"] = [](EmuDb &db, int64_t id, sol::variadic_args va) {
        return db.RenderThumbnailForAsset(id, va.size() > 0 ? va.get<int>(0) : 0);
    };
    emuDbType["RetrieveAssetData"] = [bytesToString](EmuDb &db, int64_t id, int version) -> std::tuple<SqlDb::Response, std::string, std::string> {
        std::vector<unsigned char> data;
        std::string hash;
        SqlDb::Response res = db.RetrieveAssetData(id, version, &data, &hash);
        return {res, bytesToString(data), hash};
    };

    /* Universe/place lookups */
    emuDbType["GetUniverseIdForPlace"] = &EmuDb::GetUniverseIdForPlace;
    emuDbType["GetStartPlaceIdForUniverse"] = &EmuDb::GetStartPlaceIdForUniverse;
    emuDbType["GetItemName"] = &EmuDb::GetItemName;
    emuDbType["GetCreatorUserId"] = &EmuDb::GetCreatorUserId;
    emuDbType["SearchAssetIds"] = &EmuDb::SearchAssetIds;
    emuDbType["GetAssetSummary"] = [assetSummaryToTable](EmuDb &db, int64_t id) -> sol::object {
        std::optional<EmuDb::AssetSummary> summary = db.GetAssetSummary(id);
        if (!summary.has_value())
            return sol::lua_nil;
        return assetSummaryToTable(summary.value());
    };

    /* Bundles/outfits/characters */
    emuDbType["AddAssetToBundle"] = &EmuDb::AddAssetToBundle;
    emuDbType["RemoveAssetFromBundle"] = &EmuDb::RemoveAssetFromBundle;
    emuDbType["AddAssetToOutfit"] = &EmuDb::AddAssetToOutfit;
    emuDbType["RemoveAssetFromOutfit"] = &EmuDb::RemoveAssetFromOutfit;
    emuDbType["AddAssetToUserCharacter"] = &EmuDb::AddAssetToUserCharacter;
    emuDbType["RemoveAssetFromUserCharacter"] = &EmuDb::RemoveAssetFromUserCharacter;
    emuDbType["RetrieveImageData"] = [bytesToString](EmuDb &db, ItemType type, int64_t id) -> std::string {
        return bytesToString(db.RetrieveImageData(type, id));
    };

    auto emuDbMgrType = new_usertype<EmuDbManager>("EmuDbManager", sol::no_constructor);
    emuDbMgrType["GetMasterDatabase"] = &EmuDbManager::GetMasterDatabase;
    // Return a plain Lua array (1-indexed) of EmuDb* so it can be iterated with
    // ipairs(); a raw std::vector return is exposed as userdata that ipairs rejects.
    emuDbMgrType["GetMountedDatabases"] = [this](EmuDbManager &mgr) -> sol::table {
        sol::table t = create_table();
        for (EmuDb *db : mgr.GetMountedDatabases())
            t.add(db);
        return t;
    };
    emuDbMgrType["GetFirstDbWhereItemExists"] = &EmuDbManager::GetFirstDbWhereItemExists;
    emuDbMgrType["GetDbFromFileName"] = &EmuDbManager::GetDbFromFileName;
    emuDbMgrType["GetUniverseIdForPlace"] = &EmuDbManager::GetUniverseIdForPlace;
    emuDbMgrType["GetStartPlaceIdForUniverse"] = &EmuDbManager::GetStartPlaceIdForUniverse;
    emuDbMgrType["GetItemName"] = &EmuDbManager::GetItemName;
    emuDbMgrType["GetCreatorUserId"] = &EmuDbManager::GetCreatorUserId;
    emuDbMgrType["SearchAssetIds"] = &EmuDbManager::SearchAssetIds;
    emuDbMgrType["GetAssetSummary"] = [assetSummaryToTable](EmuDbManager &mgr, int64_t id) -> sol::object {
        std::optional<EmuDb::AssetSummary> summary = mgr.GetAssetSummary(id);
        if (!summary.has_value())
            return sol::lua_nil;
        return assetSummaryToTable(summary.value());
    };
    emuDbMgrType["RetrieveImageData"] = [bytesToString](EmuDbManager &mgr, ItemType type, int64_t id) -> std::string {
        return bytesToString(mgr.RetrieveImageData(type, id));
    };
    emuDbMgrType["RetrieveAssetData"] = [bytesToString](EmuDbManager &mgr, int64_t id, int version) -> std::tuple<SqlDb::Response, std::string, std::string> {
        std::vector<unsigned char> data;
        std::string hash;
        SqlDb::Response res = mgr.RetrieveAssetData(id, version, &data, &hash);
        return {res, bytesToString(data), hash};
    };

    auto srvType = new_usertype<HttpServer>("HttpServer", sol::no_constructor);
    srvType["new"] = [this](std::string logName) {
        if (!logName.empty())
            return std::make_unique<HttpServer>(mCore, logName);
        else return std::make_unique<HttpServer>(mCore);
    };
    srvType["Start"] = &HttpServer::Start;
    srvType["Stop"] = &HttpServer::Stop;
    srvType["StartSecure"] = &HttpServer::StartSecure;
    srvType["StopSecure"] = &HttpServer::StopSecure;
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
    srvEmuType["GetRunningInstances"] = [this](ServerEmulator &emu) {
        sol::table tbl = create_table();
        int idx = 1;
        for (const auto &inst : emu.GetRunningInstances()) {
            sol::table row = create_table();
            row["Pid"] = inst.Pid;
            row["Side"] = EngineSideAsString(inst.Side);
            row["Version"] = inst.Version;
            row["Ip"] = inst.Ip;
            if (inst.Port.has_value()) row["Port"] = inst.Port.value();
            if (inst.PlaceId.has_value()) row["PlaceId"] = inst.PlaceId.value();
            row["FirstSeen"] = static_cast<int64_t>(inst.FirstSeen);
            row["LastSeen"] = static_cast<int64_t>(inst.LastSeen);
            tbl[idx++] = row;
        }
        return tbl;
    };
    srvEmuType["GetRunningGameServers"] = [this](ServerEmulator &emu) {
        sol::table tbl = create_table();
        int idx = 1;
        for (const auto &inst : emu.GetRunningGameServers()) {
            sol::table row = create_table();
            row["Pid"] = inst.Pid;
            row["Version"] = inst.Version;
            row["Ip"] = inst.Ip;
            if (inst.Port.has_value()) row["Port"] = inst.Port.value();
            if (inst.PlaceId.has_value()) row["PlaceId"] = inst.PlaceId.value();
            row["FirstSeen"] = static_cast<int64_t>(inst.FirstSeen);
            row["LastSeen"] = static_cast<int64_t>(inst.LastSeen);
            tbl[idx++] = row;
        }
        return tbl;
    };

    auto cmdCtxType = new_usertype<CommandContext>("CommandContext", sol::no_constructor);
    cmdCtxType["Reply"] = [](CommandContext& ctx, std::string msg) {
        ctx.Reply(msg);
    };
    cmdCtxType["Args"] = sol::property([](CommandContext& ctx) {
        return ctx.Args;
    });

    auto consoleType = new_usertype<Console>("Console");
    consoleType["RegisterCommand"] = [](Console& console, std::string name, sol::protected_function func, std::string desc) {
        auto cmd = std::make_unique<FuncCommand>(func);
        console.RegisterCommand(name, std::move(cmd), desc);
    };

    auto netClientType = new_usertype<LuaNetClient>("NetClient");
    netClientType["SetTimeout"] = [](LuaNetClient& cli, long secs) { cli.timeout = secs; };
    netClientType["SetHeader"] = [](LuaNetClient& cli, std::string name, std::string value) {
        cli.defaultHeaders.push_back({std::move(name), std::move(value)});
    };
    auto doRequest = [](const std::string &method, const std::string &url, const std::string &body,
                        const std::string &contentType,
                        const std::vector<std::pair<std::string, std::string>> &headers,
                        long timeout) -> cpr::Response {
        cpr::Session session;
        session.SetUrl(cpr::Url{url});
        cpr::Header h;
        for (const auto &[name, value] : headers)
            h[name] = value;
        if (!contentType.empty())
            h["Content-Type"] = contentType;
        session.SetHeader(h);
        session.SetTimeout(cpr::Timeout{std::chrono::seconds(timeout)});
        curl_easy_setopt(session.GetCurlHolder()->handle, CURLOPT_SSL_OPTIONS, (long)CURLSSLOPT_NATIVE_CA);
        if (method == "POST") {
            session.SetBody(cpr::Body{body});
            return session.Post();
        }
        return session.Get();
    };
    auto buildResponse = [this](const cpr::Response &res) -> sol::table {
        sol::table tbl = create_table();
        tbl["Ok"] = (res.error.code == cpr::ErrorCode::OK);
        tbl["Status"] = static_cast<int>(res.status_code);
        tbl["Body"] = res.text;
        if (res.error.code != cpr::ErrorCode::OK)
            tbl["Error"] = res.error.message;
        return tbl;
    };
    netClientType["Get"] = [doRequest, buildResponse](LuaNetClient &c, std::string url) -> sol::table {
        return buildResponse(doRequest("GET", url, "", "", c.defaultHeaders, c.timeout));
    };
    netClientType["Post"] = [doRequest, buildResponse](LuaNetClient &c, std::string url, std::string body, std::string contentType) -> sol::table {
        return buildResponse(doRequest("POST", url, body, contentType, c.defaultHeaders, c.timeout));
    };
    netClientType["PostJson"] = [this, doRequest, buildResponse](LuaNetClient &c, std::string url, sol::table tbl) -> sol::table {
        sol::protected_function jsonEncode = (*this)["json"]["encode"];
        std::string body;
        auto encRes = jsonEncode(tbl);
        if (encRes.valid()) body = encRes.get<std::string>();
        return buildResponse(doRequest("POST", url, body, "application/json", c.defaultHeaders, c.timeout));
    };
    netClientType["Request"] = [doRequest, buildResponse](LuaNetClient &c, sol::table params) -> sol::table {
        std::string url = params.get_or<std::string>("Url", "");
        std::string method = params.get_or<std::string>("Method", "GET");
        std::string body = params.get_or<std::string>("Body", "");
        std::string contentType = params.get_or<std::string>("ContentType", "");
        std::vector<std::pair<std::string, std::string>> headers = c.defaultHeaders;
        sol::optional<sol::table> extraHeaders = params["Headers"];
        if (extraHeaders) {
            for (const auto &kv : *extraHeaders)
                headers.push_back({kv.first.as<std::string>(), kv.second.as<std::string>()});
        }
        return buildResponse(doRequest(method, url, body, contentType, headers, c.timeout));
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
        if (res != Lhp::RenderResponse::Success && res != Lhp::RenderResponse::ExitCalled) {
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
        
        UrlContext ctx {};
        sol::object scriptObj = env["script"];
        if (scriptObj.get_type() == sol::type::userdata) {
            LuaScript& script = scriptObj.as<LuaScript&>();
            ctx.DefaultProtocolType = script.GetUrl().GetProtocol();
            ctx.DefaultHostName = script.GetUrl().GetHostName();
            ctx.Cwd = script.GetUrl().GetCwd();
        }

        Url url(fileLocation, ctx);

        std::string output;
        Lhp::RenderResponse res = mLhp.Render(globalsEnv, url, &output);
        if (res != Lhp::RenderResponse::Success && res != Lhp::RenderResponse::ExitCalled) {
            luaL_error(L, "failed to render page using lhp");
        }
        return output;
    });
    set("lhp", lhpLib);

    sol::table hashLib = create_table();
    hashLib.set_function("GenerateToken", [](sol::this_state state) -> std::string {
        lua_State* L = state;
        unsigned char tokenBytes[32];
        if (RAND_bytes(tokenBytes, sizeof(tokenBytes)) != 1) {
            luaL_error(L, "failed to generate token");
            return "";
        }
        return HexEncode(tokenBytes, sizeof(tokenBytes));
    });
    hashLib.set_function("HashPassword", [this](sol::this_state state, std::string password) -> sol::object {
        lua_State* L = state;
        constexpr size_t SALT_LEN = 16, HASH_LEN = 32;
        unsigned char salt[SALT_LEN];
        if (RAND_bytes(salt, sizeof(salt)) != 1) {
            luaL_error(L, "failed to generate salt");
            return sol::lua_nil;
        }
        unsigned char hash[HASH_LEN];
        if (!Argon2idHash(password, salt, SALT_LEN, hash, HASH_LEN)) {
            luaL_error(L, "failed to hash password");
            return sol::lua_nil;
        }
        sol::table result = this->create_table();
        result["hash"] = HexEncode(hash, HASH_LEN);
        result["salt"] = HexEncode(salt, SALT_LEN);
        return result;
    });
    hashLib.set_function("VerifyPassword", [](sol::this_state state, std::string password, std::string hashHex, std::string saltHex) -> bool {
        lua_State* L = state;
        constexpr size_t HASH_LEN = 32;
        std::vector<unsigned char> salt;
        if (!HexDecode(saltHex, salt) || salt.size() != 16) {
            luaL_error(L, "malformed salt");
            return false;
        }
        unsigned char hash[HASH_LEN];
        if (!Argon2idHash(password, salt.data(), salt.size(), hash, HASH_LEN)) {
            luaL_error(L, "failed to hash password");
            return false;
        }
        std::string computed = HexEncode(hash, HASH_LEN);
        if (computed.size() != hashHex.size()) return false;
        return CRYPTO_memcmp(computed.data(), hashHex.data(), computed.size()) == 0;
    });
    hashLib.set_function("Sha256", [](const std::string &data) -> std::string {
        unsigned char digest[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(data.data()), data.size(), digest);
        return HexEncode(digest, sizeof(digest));
    });
    set("hash", hashLib);

    sol::table coreLib = create_table();
    coreLib.set_function("GetVersion", []() -> const char* {
        return NOOBWARRIOR_VERSION;
    });
    coreLib.set_function("GetInstallDataDir", [this]() -> VirtualFileSystem* {
        return mCore->GetInstallDataVfs();
    });
    coreLib.set_function("GetUserDataDir", [this]() -> VirtualFileSystem* {
        return mCore->GetUserDataVfs();
    });
    coreLib.set_function("GetPluginDataDir", [this]() -> VirtualFileSystem* {
        return mCore->GetPluginDataVfs();
    });
    coreLib["ConsoleAdded"] = mCore->GetConsoleAddedSignal();
    coreLib.set_function("GetEmuDbManager", [this]() -> EmuDbManager* {
        return mCore->GetEmuDbManager();
    });
    coreLib.set_function("GetMasterDatabase", [this]() -> EmuDb* {
        return mCore->GetEmuDbManager()->GetMasterDatabase();
    });
    // Builds a /v1.1/avatar-fetch body for a local user id from the master DB. Lets the master-server
    // plugin serve a user's avatar over federation without reimplementing the appearance logic.
    coreLib.set_function("BuildAvatarFetchJson", [this](int64_t userId) -> std::string {
        return AvatarAppearance::BuildAvatarFetchJsonForUser(mCore, userId).dump();
    });
    set("core", coreLib);

    sol::table urlLib = create_table();
    urlLib.set_function("GetProtocol", [this](const std::string &url) -> ProtocolType {
        return Url(url).GetProtocol();
    });
    urlLib.set_function("GetProtocolString", [this](const std::string &url) -> std::string {
        return Url(url).GetProtocolString();
    });
    urlLib.set_function("GetHostName", [this](const std::string &url) -> std::string {
        return Url(url).GetHostName();
    });
    urlLib.set_function("GetCwd", [this](const std::string &url) -> std::string {
        return Url(url).GetCwd();
    });
    urlLib.set_function("GetDirectory", [this](const std::string &url) -> std::string {
        return Url(url).GetDirectory();
    });
    urlLib.set_function("GetFileName", [this](const std::string &url) -> std::string {
        return Url(url).GetFileName();
    });
    urlLib.set_function("Resolve", [this](const std::string &url) -> std::string {
        return Url(url).Resolve();
    });
    urlLib.set_function("ResolveWithoutProtocol", [this](const std::string &url) -> std::string {
        return Url(url).ResolveWithoutProtocol();
    });
    urlLib.set_function("ResolveAsPath", [this](const std::string &url) -> std::string {
        return Url(url).ResolveAsPath();
    });
    // Resolves a URL (e.g. plugindata://master-server@.../master.nwdb) to a real local path.
    // Useful for opening a plugin-owned SqlDb by path.
    urlLib.set_function("ResolveAsLocalPath", [this](const std::string &url) -> std::string {
        return Url(url).ResolveAsLocalPath(mCore).string();
    });
    // Returns the VFS backing a URL (works for plugin:// and plugindata://), or nil.
    urlLib.set_function("GetVfs", [this](const std::string &url) -> VirtualFileSystem* {
        return Url(url).GetVfs(mCore);
    });
    // Resolves a (possibly relative or host-less) URL against the script that called into the
    // current plugin, like the nearest function on the Lua call stack that belongs to a
    // different plugin than the immediate caller. This lets infrastructure plugins like
    // http-base resolve URLs authored by the plugin that invoked them (e.g. a sitemap defined
    // in master-server's main.lua, using paths like "/src/index.lhp") against that plugin
    // rather than against http-base itself. Absolute URLs (those carrying a protocol) pass
    // through unchanged.
    // (This function was generated by claude, it looks fucking horrible. Why am i pushing this into master?)
    urlLib.set_function("ResolveFromCaller", [](sol::this_state state, const std::string &path) -> std::string {
        lua_State* L = state;

        auto scriptAtLevel = [L](int level) -> LuaScript* {
            lua_Debug ar;
            if (lua_getstack(L, level, &ar) == 0)
                return nullptr;
            if (lua_getinfo(L, "f", &ar) == 0)
                return nullptr; // pushes the function
            LuaScript* result = nullptr;
            if (lua_isfunction(L, -1)) {
                lua_getfenv(L, -1); // pushes the function's environment (sol stores `script` here)
                if (lua_istable(L, -1)) {
                    sol::table env(L, -1);
                    result = env["script"].get_or<LuaScript*>(nullptr);
                }
                lua_pop(L, 1); // env
            }
            lua_pop(L, 1); // function
            return result;
        };

        // Level 1 is the immediate Lua caller (the plugin doing the resolving, e.g. http-base).
        LuaScript* self = scriptAtLevel(1);
        std::string selfHost = self != nullptr ? self->GetUrl().GetHostName() : std::string();

        LuaScript* owner = self;
        for (int level = 2; ; level++) {
            lua_Debug probe;
            if (lua_getstack(L, level, &probe) == 0)
                break; // hit the bottom of the stack without finding a foreign caller
            LuaScript* candidate = scriptAtLevel(level);
            if (candidate != nullptr && candidate->GetUrl().GetHostName() != selfHost) {
                owner = candidate;
                break;
            }
        }

        UrlContext ctx {};
        if (owner != nullptr) {
            Url ownerUrl = owner->GetUrl();
            ctx = {
                .Cwd = ownerUrl.GetDirectory(),
                .DefaultProtocolType = ownerUrl.GetProtocol(),
                .DefaultHostName = ownerUrl.GetHostName(),
            };
        }
        return Url(path, ctx).Resolve();
    });
    set("url", urlLib);
    
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
