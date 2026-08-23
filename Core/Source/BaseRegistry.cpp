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
// File: BaseRegistry.cpp
// Started by: Hattozo
// Started on: 6/23/2025
// Description:
#include <NoobWarrior/BaseRegistry.h>
#include <NoobWarrior/NoobWarrior.h>

#include <cstdio>
#include <sstream>
#include <utility>
#include <vector>

#include "Lua/files/registry_metatable.lua.inc.cpp"
#include "Lua/files/serpent.lua.inc.cpp"

NoobWarrior::BaseRegistry::BaseRegistry(std::string globalName, std::filesystem::path filePath, LuaState* lua) :
    mGlobalName(std::move(globalName)),
    mFilePath(std::move(filePath)),
    mLua(lua)
{}

NoobWarrior::RegistryResponse NoobWarrior::BaseRegistry::Open() {
    sol::state& lua = *mLua;
    mLastError = "";

    if (!std::filesystem::exists(mFilePath)) {
        // file doesn't exist, create a new empty table.
        lua[mGlobalName] = lua.create_table();
    } else {
        sol::load_result chunk = lua.load_file(mFilePath.generic_string());
        if (!chunk.valid()) {
            sol::error err = chunk;
            mLastError = err.what();
            switch (chunk.status()) {
                case sol::load_status::syntax: return RegistryResponse::SyntaxError;
                case sol::load_status::memory: return RegistryResponse::MemoryError;
                case sol::load_status::file:   return RegistryResponse::CantReadFile;
                default:                       return RegistryResponse::Failed;
            }
        }

        sol::protected_function loaded = chunk;
        sol::protected_function_result res = loaded();
        if (!res.valid()) {
            sol::error err = res;
            mLastError = err.what();
            return RegistryResponse::ErrorDuringExecution;
        }

        sol::object ret = res;
        if (ret.get_type() != sol::type::table) {
            mLastError = std::format("expected table, got {}",
                lua_typename(lua.lua_state(), static_cast<int>(ret.get_type())));
            return RegistryResponse::ReturningWrongType;
        }

        // set our global as what our registry file returned, a table.
        lua[mGlobalName] = ret;
    }

    // Attach a metatable to our registry table that will make it so that if you index anything in it, it will make it a table if its nil.
    // This is really good for doing assignments that require access to a lot of tables like "registry.gui.database_editor.content_browser.size.x = 200"
    // because it will auto-create each table in the process without having to manually do it yourself.
    //
    // Of course this can get messy, so when we serialize the table we remove any empty tables beforehand so that it
    // doesn't look godawful when you open it in your text editor
    char buf[2048];
    snprintf(buf, sizeof(buf), registry_metatable_lua, mGlobalName.c_str());
    if (sol::protected_function_result metaRes = lua.safe_script(buf, sol::script_pass_on_error); !metaRes.valid()) {
        sol::error err = metaRes;
        mLastError = err.what();
        return RegistryResponse::ErrorDuringExecution;
    }

    Out("Registry", "Opened registry");
    return RegistryResponse::Success;
}

std::optional<sol::object> NoobWarrior::BaseRegistry::RawGetKeyObject(const std::string &key) {
    if (key.empty())
        return std::nullopt;

    sol::state &lua = *mLua;
    sol::object cursor = lua[mGlobalName];

    size_t start = 0;
    while (start <= key.size()) {
        const size_t dot = key.find('.', start);
        const std::string segment = key.substr(start, dot == std::string::npos ? std::string::npos : dot - start);
        if (segment.empty())
            return std::nullopt;

        // Every segment before the last must itself be a table for the walk to continue.
        if (cursor.get_type() != sol::type::table)
            return std::nullopt;

        // raw_get, not operator[], so a missing segment stays missing instead of being created.
        sol::object next = cursor.as<sol::table>().raw_get<sol::object>(segment);
        if (next == sol::lua_nil)
            return std::nullopt;
        cursor = next;

        if (dot == std::string::npos)
            break;
        start = dot + 1;
    }

    return cursor;
}

bool NoobWarrior::BaseRegistry::HasKey(const std::string &key) {
    return RawGetKeyObject(key).has_value();
}

NoobWarrior::RegistryResponse NoobWarrior::BaseRegistry::Save() {
    sol::state& lua = *mLua;

    // First lets remove all empty tables in our registry table.
    // Since our metatable will automatically set any indexed nil value to a table, it creates a lot of clutter and junk
    std::string pruneSrc = std::format(R"(
        local function prune(tbl)
            for k, v in pairs(tbl) do
                if type(v) == "table" then
                    prune(v)
                    if next(v) == nil then
                        rawset(tbl, k, nil)
                    end
                end
            end
        end
        prune({});
    )", mGlobalName);

    if (sol::protected_function_result pruneRes = lua.safe_script(pruneSrc, sol::script_pass_on_error); !pruneRes.valid()) {
        sol::error err = pruneRes;
        mLastError = err.what();
        return RegistryResponse::ErrorDuringExecution;
    }

    // Load in serpent.lua through our header file that embeds it into the program.
    // It's a lua serializer that reconstructs a source-code version of the table.
    sol::load_result serpentChunk = lua.load(serpent_lua);
    if (!serpentChunk.valid()) {
        sol::error err = serpentChunk;
        mLastError = err.what();
        switch (serpentChunk.status()) {
            case sol::load_status::syntax: return RegistryResponse::SyntaxError;
            case sol::load_status::memory: return RegistryResponse::MemoryError;
            case sol::load_status::file:   return RegistryResponse::CantReadFile;
            default:                       return RegistryResponse::Failed;
        }
    }

    sol::protected_function serpentLoad = serpentChunk;
    sol::protected_function_result serpentRes = serpentLoad();
    if (!serpentRes.valid()) {
        sol::error err = serpentRes;
        mLastError = err.what();
        return RegistryResponse::ErrorDuringExecution;
    }

    sol::table serpent = serpentRes;
    sol::protected_function block = serpent["block"];

    sol::table opts = lua.create_table();
    opts["indent"] = "    ";
    opts["comment"] = false;

    sol::object registryTable = lua[mGlobalName];
    sol::protected_function_result blockRes = block(registryTable, opts);
    if (!blockRes.valid()) {
        sol::error err = blockRes;
        mLastError = err.what();
        return RegistryResponse::ErrorDuringExecution;
    }

    std::string serialized = ApplyKeyComments(blockRes.get<std::string>());

    // open file and write to it
    std::ofstream fileOutput(mFilePath);
    if (fileOutput.fail())
        return RegistryResponse::CantReadFile;
    fileOutput << "return " << serialized;

    return RegistryResponse::Success;
}

// this doesn't really do all that much anymore, it's just an alias
NoobWarrior::RegistryResponse NoobWarrior::BaseRegistry::Close() {
    const RegistryResponse res = Save();
    if (res == RegistryResponse::Success)
        Out("Registry", "Closed registry");
    return res;
}

std::string NoobWarrior::BaseRegistry::GetLuaError() {
    return mLastError;
}

void NoobWarrior::BaseRegistry::SetKeyComment(const char *key, const char *comment) {
    if (key == nullptr || comment == nullptr)
        return;

    std::string keyStr(key);
    if (keyStr.empty()) {
        Out("BaseRegistry", "Error setting comment: key is empty");
        return;
    }
    if (keyStr.starts_with('.') || keyStr.ends_with('.')) {
        Out("BaseRegistry", "Error setting comment for key \"{}\": key cannot start or end with a period", keyStr);
        return;
    }

    mKeyComments[keyStr] = comment;
}

std::string NoobWarrior::BaseRegistry::ApplyKeyComments(const std::string &serialized) {
    if (mKeyComments.empty())
        return serialized;

    constexpr int kIndentUnit = 4;

    std::vector<std::string> pathStack;
    std::vector<std::string> outLines;

    std::istringstream in(serialized);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        const size_t indentSpaces = line.find_first_not_of(' ');
        if (indentSpaces == std::string::npos) { // blank line
            outLines.push_back(line);
            continue;
        }

        std::string content = line.substr(indentSpaces);
        std::string rtrim = content;
        while (!rtrim.empty() && (rtrim.back() == ' ' || rtrim.back() == '\t'))
            rtrim.pop_back();

        const int depth = static_cast<int>(indentSpaces / kIndentUnit);
        
        if (rtrim == "}" || rtrim == "},") {
            if (depth >= 1 && static_cast<int>(pathStack.size()) >= depth)
                pathStack.resize(depth - 1);
            outLines.push_back(line);
            continue;
        }

        const bool isOpener = !rtrim.empty() && rtrim.back() == '{';

        std::string key;
        bool hasKey = false;
        if (const size_t eq = content.find(" = "); eq != std::string::npos) {
            key = content.substr(0, eq);
            hasKey = true;
        }

        if (depth >= 1 && static_cast<int>(pathStack.size()) > depth - 1)
            pathStack.resize(depth - 1);

        if (hasKey) {
            std::string full;
            for (const std::string &seg : pathStack) {
                if (!full.empty()) full += '.';
                full += seg;
            }
            if (!full.empty()) full += '.';
            full += key;

            if (const auto it = mKeyComments.find(full); it != mKeyComments.end()) {
                const std::string indentStr(indentSpaces, ' ');
                const std::string &c = it->second;
                size_t start = 0;
                while (true) {
                    const size_t nl = c.find('\n', start);
                    outLines.push_back(indentStr + "-- " +
                        c.substr(start, nl == std::string::npos ? std::string::npos : nl - start));
                    if (nl == std::string::npos) break;
                    start = nl + 1;
                }
            }
        }

        outLines.push_back(line);

        if (isOpener)
            pathStack.push_back(hasKey ? key : std::string());
    }

    std::string result;
    for (size_t i = 0; i < outLines.size(); ++i) {
        result += outLines[i];
        if (i + 1 < outLines.size())
            result += '\n';
    }
    return result;
}

std::string NoobWarrior::BaseRegistry::GetGlobalName() {
    return mGlobalName;
}
