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
// File: Plugin.cpp
// Started by: Hattozo
// Started on: 12/3/2025
// Description:
#include <NoobWarrior/Plugin.h>
#include <NoobWarrior/PluginDataModel.h>
#include <NoobWarrior/Lua/LuaState.h>
#include <NoobWarrior/Lua/LuaScript.h>
#include <NoobWarrior/Url.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/FileSystem/VirtualFileSystem.h>

#include "Lua/files/plugin_env_metatable.lua.inc.cpp"

#include <lua.hpp>
#include <sol/sol.hpp>
#include <memory>
#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <iterator>
#include <limits>

#define ERR_LOG_TEMPLATE "Failed to load plugin \"{}\" because "
#define PLUGIN_OUT(format, ...) \
    mCore->Out("Plugin", "[{}] " format, identifier __VA_OPT__(,) __VA_ARGS__);

using namespace NoobWarrior;

static std::string LuaLongString(const std::string &value) {
    std::string equals;
    while (value.find("]" + equals + "]") != std::string::npos ||
           value.ends_with("]" + equals)) {
        equals += '=';
    }
    return "[" + equals + "[" + value + "]" + equals + "]";
}

static std::string SandboxedEngineAutorun(const std::string &source) {
    // Keep plugin text at the top level of its own Script. Putting it inside a generated function
    // would let malformed or hostile text close that function before setfenv is applied. The
    // setup locals also go out of lexical scope before plugin text begins.
    return "do\n"
        "local host = getfenv(1)\n"
        "local sandbox = {}\n"
        "for key, value in pairs(host) do\n"
        "if key ~= 'script' and key ~= 'getfenv' and key ~= 'setfenv' then "
        "sandbox[key] = value end\n"
        "end\n"
        "sandbox._G = sandbox\n"
        "setmetatable(sandbox, {__index = function(_, key)\n"
        "if key == 'script' or key == 'getfenv' or key == 'setfenv' then return nil end\n"
        "return host[key]\n"
        "end, __metatable = false})\n"
        "setfenv(1, sandbox)\n"
        "end\n" + source + "\n";
}

/* NOTE: File names are relative to the path of the plugins folder in noobWarrior's user directory folder. */
Plugin::Plugin(const std::filesystem::path &filePath, Core* core) :
    mResponse(Response::Failed),
    mCore(core),
    mFilePath(filePath),
    mVfs(nullptr)
{
    if (!mCore->GetLuaState()->Opened()) {
        mCore->Out("Plugin", ERR_LOG_TEMPLATE "the Lua subsystem is not open! Perhaps the plugin was initialized too early?", GetFileName());
        mResponse = Response::Failed;
        return;
    }

    // Use a virtual filesystem so that we can use both compressed archives and regular folders.
    VirtualFileSystem::Response fsRes = VirtualFileSystem::New(&mVfs, mFilePath);
    if (fsRes != VirtualFileSystem::Response::Success || mVfs == nullptr) {
        mCore->Out("Plugin", ERR_LOG_TEMPLATE "the virtual filesystem failed to initialize.", GetFileName());
        mResponse = Response::Failed;
        return;
    }

    if (!mVfs->EntryExists("/plugin.lua")) {
        mCore->Out("Plugin", ERR_LOG_TEMPLATE "its root directory does not contain a plugin.lua file.", GetFileName());
        mResponse = Response::Failed;
        return;
    }

    std::string pluginLuaString;

    mVfsHandle = mVfs->OpenHandle("/plugin.lua");
    std::string buf;
    while (mVfs->ReadHandleLine(mVfsHandle, &buf))
        pluginLuaString.append(buf + '\n');

    sol::protected_function_result res = mCore->GetLuaState()->safe_script(pluginLuaString);
    if (!res.valid()) {
        sol::error err = res;
        mCore->Out("Plugin", ERR_LOG_TEMPLATE "plugin.lua failed with error: {}", GetFileName(), err.what());
        mResponse = Response::Failed;
        return;
    }
    if (res.get_type() != sol::type::table) {
        mCore->Out("Plugin", ERR_LOG_TEMPLATE "plugin.lua did not return a table.", GetFileName());
        mResponse = Response::Failed;
        return;
    }

    mManifestTbl = res.get<sol::table>();
    auto identifier = mManifestTbl.get<std::optional<std::string>>("identifier");
    auto title = mManifestTbl.get<std::optional<std::string>>("title");

    if (identifier == std::nullopt) {
        mCore->Out("Plugin", ERR_LOG_TEMPLATE "it does not have an identifier set in plugin.lua.", GetFileName());
        mResponse = Response::Failed;
        return;
    }

    if (title == std::nullopt) {
        mCore->Out("Plugin", ERR_LOG_TEMPLATE "it does not have a title set in plugin.lua.", GetFileName());
        mResponse = Response::Failed;
        return;
    }

    mResponse = Response::Success;
    OpenEnv();
}

Plugin::~Plugin() {
    if (Fail())
        return;
    mScripts.clear();
    if (mVfs != nullptr) {
        mVfs->CloseHandle(mVfsHandle);
        VirtualFileSystem::Free(mVfs);
    }
}

Plugin::Response Plugin::Execute() {
    if (Fail()) {
        mCore->Out("Plugin", "Plugin::Execute() called but plugin \"{}\" failed to initialize!", GetFileName());
        return Plugin::Response::Failed;
    }

    auto identifier = mManifestTbl.get<std::string>("identifier");

    PLUGIN_OUT("Executing...")

    auto autorunTbl = mManifestTbl.get<std::optional<sol::table>>("autorun");
    if (autorunTbl != std::nullopt) {
        for (int i = 1; i <= autorunTbl->size(); i++) {
            auto val = autorunTbl->get<sol::object>(i);
            if (!val.is<std::string>()) {
                PLUGIN_OUT("Value in index {} in autorun is not string!", i)
                continue;
            }
            auto script = std::make_unique<LuaScript>(mCore->GetLuaState(), mEnv, (Url(val.as<std::string>(), {
                .DefaultProtocolType = ProtocolType::Plugin,
                .DefaultHostName = identifier
            })));
            if (!script->Fail()) {
                script->Execute();
                mScripts.push_back(std::move(script));
            }
        }
    }

    return Response::Success;
}

bool Plugin::Fail() {
    return mResponse != Response::Success;
}

Plugin::Response Plugin::GetInitResponse() {
    return mResponse;
}

VirtualFileSystem* Plugin::GetVfs() {
    return mVfs;
}

std::filesystem::path Plugin::GetFilePath() {
    return mFilePath;
}

std::string Plugin::GetFileName() {
    std::string str = mFilePath.string();
    std::string::size_type last_slash = str.find_last_of("/");
	return last_slash != std::string::npos ? str.substr(last_slash + 1) : str;
}

std::string Plugin::GetIdentifier() {
    return mManifestTbl.get_or<std::string>("identifier", "");
}

std::vector<Plugin::DeclaredDatabase> Plugin::GetDeclaredDatabases() {
    std::vector<DeclaredDatabase> databases;
    if (Fail())
        return databases;

    const std::string identifier = GetIdentifier();
    auto databasesTbl = mManifestTbl.get<std::optional<sol::table>>("databases");
    if (!databasesTbl)
        return databases;

    for (std::size_t i = 1; i <= databasesTbl->size(); i++) {
        sol::object value = databasesTbl->get<sol::object>(i);
        if (!value.is<sol::table>()) {
            PLUGIN_OUT("Value in index {} in databases is not a table!", i)
            continue;
        }

        sol::table entry = value.as<sol::table>();
        auto url = entry.get<std::optional<std::string>>("url");
        if (!url || url->empty()) {
            PLUGIN_OUT("Value in index {} in databases has no url string!", i)
            continue;
        }

        // Same resolution autorun uses, so a bare "databases/x.nwdb" means this plugin's own file
        // while a full URL (userdata://...) still addresses whatever it names.
        Url resolved(*url, {
            .DefaultProtocolType = ProtocolType::Plugin,
            .DefaultHostName = identifier
        });
        if (resolved.Fail()) {
            PLUGIN_OUT("Url \"{}\" in databases index {} is not valid!", *url, i)
            continue;
        }

        DeclaredDatabase declared;
        declared.SourceUrl = resolved.Resolve();
        declared.OwnerIdentifier = identifier;
        declared.Required = entry.get<sol::optional<bool>>("required").value_or(false);
        declared.Writable = entry.get<sol::optional<bool>>("writable").value_or(false);
        databases.push_back(std::move(declared));
    }

    return databases;
}

bool Plugin::IsDirectoryBacked() {
    // Matches how VirtualFileSystem::GetFormatFromPath picks a backend: a directory gets
    // StdFileSystem (real paths on disk), anything else is treated as a zip.
    std::error_code ec;
    return std::filesystem::is_directory(mFilePath, ec) && !ec;
}

bool Plugin::ExtractFile(const std::string &path, const std::filesystem::path &destination) {
    if (Fail() || mVfs == nullptr)
        return false;

    // Same normalization and traversal rejection as ReadFile; only the destination differs.
    std::string normalized = path;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    if (!normalized.starts_with('/'))
        normalized.insert(normalized.begin(), '/');
    if (normalized.find("/../") != std::string::npos || normalized.ends_with("/..") ||
        !mVfs->EntryExists(normalized)) {
        return false;
    }

    FSEntryInfo info = mVfs->GetEntryFromPath(normalized);
    if (info.Failed || info.Type != FSEntryInfo::Type::File)
        return false;

    std::error_code ec;
    std::filesystem::create_directories(destination.parent_path(), ec);

    FSEntryHandle handle = mVfs->OpenHandle(normalized);
    if (handle == 0)
        return false;

    std::ofstream out(destination, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        mVfs->CloseHandle(handle);
        return false;
    }

    // Stream it: a shipped database can be hundreds of megabytes and there is no reason to hold a
    // second copy of it in memory just to write it back out.
    constexpr unsigned int kChunkSize = 1u << 20;
    bool ok = true;
    std::uintmax_t remaining = info.Size;
    std::vector<unsigned char> chunk;
    while (remaining > 0) {
        unsigned int want = static_cast<unsigned int>(std::min<std::uintmax_t>(remaining, kChunkSize));
        chunk.clear();
        if (!mVfs->ReadHandleChunk(handle, &chunk, want) || chunk.empty()) {
            ok = false;
            break;
        }
        out.write(reinterpret_cast<const char*>(chunk.data()), static_cast<std::streamsize>(chunk.size()));
        if (!out) {
            ok = false;
            break;
        }
        remaining -= chunk.size();
    }

    out.close();
    mVfs->CloseHandle(handle);

    if (!ok) // never leave a half-written database behind for something else to try to open
        std::filesystem::remove(destination, ec);
    return ok;
}

bool Plugin::ReadFile(const std::string &path, std::vector<unsigned char> *data) {
    if (Fail() || mVfs == nullptr || data == nullptr)
        return false;
    data->clear();

    std::string normalized = path;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    if (!normalized.starts_with('/'))
        normalized.insert(normalized.begin(), '/');
    if (normalized.find("/../") != std::string::npos || normalized.ends_with("/..") ||
        !mVfs->EntryExists(normalized)) {
        return false;
    }

    FSEntryInfo info = mVfs->GetEntryFromPath(normalized);
    if (info.Failed || info.Type != FSEntryInfo::Type::File ||
        info.Size > std::numeric_limits<unsigned int>::max()) {
        return false;
    }

    FSEntryHandle handle = mVfs->OpenHandle(normalized);
    if (handle == 0)
        return false;
    bool read = info.Size == 0 || mVfs->ReadHandleChunk(
        handle, data, static_cast<unsigned int>(info.Size));
    mVfs->CloseHandle(handle);
    return read;
}

StudioServerBootstrap Plugin::BuildStudioServerBootstrap(int64_t placeId, int64_t universeId) {
    if (Fail() || mVfs == nullptr)
        return {};

    const std::string identifier = GetIdentifier();
    StudioServerBootstrap bootstrap;
    auto dataModel = mManifestTbl.get<std::optional<sol::table>>("datamodel");
    if (dataModel) {
        PluginDataModel builder(mVfs, identifier);
        for (std::size_t i = 1; i <= dataModel->size(); i++) {
            sol::object value = dataModel->get<sol::object>(i);
            if (!value.is<sol::table>()) {
                PLUGIN_OUT("Value in index {} in datamodel is not a table!", i)
                continue;
            }

            sol::table entry = value.as<sol::table>();
            std::string side = entry.get_or<std::string>("side", "");
            std::transform(side.begin(), side.end(), side.begin(), [](unsigned char character) {
                return static_cast<char>(std::tolower(character));
            });
            // Rn client datamodels cannot really exist because we don't have a script executor at the moment.
            // Will we ever make one? I'm not sure.
            if (side != "server" && side != "shared")
                continue;

            auto directory = entry.get<std::optional<std::string>>("dir");
            if (!directory) {
                PLUGIN_OUT("Value in index {} in datamodel has no dir string!", i)
                continue;
            }

            bool selected = true;
            sol::object predicateObject = entry.get<sol::object>("predicate");
            if (predicateObject.get_type() != sol::type::none &&
                predicateObject.get_type() != sol::type::lua_nil) {
                if (!predicateObject.is<sol::protected_function>()) {
                    PLUGIN_OUT("Predicate in datamodel index {} is not a function!", i)
                    continue;
                }
                sol::table data = mCore->GetLuaState()->create_table_with(
                    "PlaceId", placeId,
                    "UniverseId", universeId,
                    "Side", "Server");
                sol::protected_function_result result =
                    predicateObject.as<sol::protected_function>()(data);
                if (!result.valid()) {
                    sol::error error = result;
                    PLUGIN_OUT("Predicate in datamodel index {} failed: {}", i, error.what())
                    continue;
                }
                selected = result.get_type() == sol::type::boolean && result.get<bool>();
            }

            if (selected) {
                StudioServerBootstrap built = builder.BuildBootstrap(*directory);
                bootstrap.Plans.insert(bootstrap.Plans.end(),
                    built.Plans.begin(), built.Plans.end());
                bootstrap.Scripts.insert(bootstrap.Scripts.end(),
                    std::make_move_iterator(built.Scripts.begin()),
                    std::make_move_iterator(built.Scripts.end()));
                bootstrap.Models.insert(bootstrap.Models.end(),
                    std::make_move_iterator(built.Models.begin()),
                    std::make_move_iterator(built.Models.end()));
            }
        }
    }

    auto engineAutorun = mManifestTbl.get<std::optional<sol::table>>("engine_autorun");
    if (!engineAutorun)
        return bootstrap;

    auto readAutorunCategory = [&](const char *category, bool server) {
        auto scripts = engineAutorun->get<std::optional<sol::table>>(category);
        if (!scripts)
            return;
        for (std::size_t i = 1; i <= scripts->size(); i++) {
            auto path = scripts->get<std::optional<std::string>>(i);
            if (!path) {
                PLUGIN_OUT("Value in index {} in engine_autorun.{} is not a string!", i,
                           category)
                continue;
            }
            std::vector<unsigned char> bytes;
            if (!ReadFile(*path, &bytes)) {
                PLUGIN_OUT("Could not read engine_autorun script \"{}\"", *path)
                continue;
            }
            std::string source(bytes.begin(), bytes.end());
            const std::string key = "__noobWarriorEngineAutorun:" + identifier + ":" +
                category + ":" + std::to_string(i);
            if (server) {
                bootstrap.Scripts.push_back({
                    key,
                    "Script",
                    SandboxedEngineAutorun(source),
                    "ServerScriptService",
                    false,
                });
            } else {
                bootstrap.Scripts.push_back({
                    key,
                    "LocalScript",
                    std::move(source),
                    "StarterPlayerScripts",
                    false,
                });
            }
        }
    };

    for (const char *category : std::array<const char *, 2> {"shared", "server"})
        readAutorunCategory(category, true);
    for (const char *category : std::array<const char *, 2> {"shared", "client"})
        readAutorunCategory(category, false);
    return bootstrap;
}

std::vector<unsigned char> Plugin::GetIconData() {
    std::vector<unsigned char> data;
    if (Fail() || mVfs == nullptr)
        return data;

    std::string iconName = mManifestTbl.get_or<std::string>("icon", "");
    if (iconName.empty())
        return data;

    std::string iconPath = "/" + iconName;
    if (!mVfs->EntryExists(iconPath))
        return data;

    FSEntryInfo info = mVfs->GetEntryFromPath(iconPath);
    if (info.Failed || !info.Exists || info.Type != FSEntryInfo::Type::File || info.Size == 0)
        return data;
    
    FSEntryHandle handle = mVfs->OpenHandle(iconPath);
    mVfs->ReadHandleChunk(handle, &data, static_cast<unsigned int>(info.Size));
    mVfs->CloseHandle(handle);
    return data;
}

const Plugin::Properties Plugin::GetProperties() {
    if (Fail()) {
        mCore->Out("Plugin", "Plugin::GetProperties() called but plugin \"{}\" failed to initialize!", GetFileName());
        return {};
    }

    Properties props {};
    props.FilePath = mFilePath;
    props.FileName = GetFileName();

    props.Identifier = mManifestTbl.get_or<std::string>("identifier", "");
    props.Title = mManifestTbl.get_or<std::string>("title", "");
    props.Version = mManifestTbl.get_or<std::string>("version", "");
    props.Description = mManifestTbl.get_or<std::string>("description", "");
    props.IconFileName = mManifestTbl.get_or<std::string>("icon", "");
    props.IconData = GetIconData();

    auto authorsTbl = mManifestTbl.get<std::optional<sol::table>>("authors");
    if (authorsTbl != std::nullopt) {
        for (int i = 1; i <= authorsTbl->size(); i++) {
            auto author = authorsTbl->get<std::optional<std::string>>(i);
            if (author != std::nullopt)
                props.Authors.push_back(*author);
        }
    }

    props.IsPrivileged = mFilePath.parent_path().filename().compare("priv-plugins") == 0;

    return props;
}

void Plugin::OpenEnv() {
    if (Fail())
        return;

    mEnv = sol::environment(*mCore->GetLuaState(), sol::create, mCore->GetLuaState()->globals());
    mEnv.set("plugin", this);
}
