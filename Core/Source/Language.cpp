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
// File: Language.cpp
// Started by: Hattozo
// Started on: 10/9/2026
// Description: Translates keys into text for the selected language
#include <NoobWarrior/Language.h>
#include <NoobWarrior/Lua/LuaState.h>
#include <NoobWarrior/Log.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

using namespace NoobWarrior;

Language::Language(LuaState* lua, std::filesystem::path dir) :
    mLua(lua),
    mDir(std::move(dir)),
    mCode(NOOBWARRIOR_DEFAULT_LANGUAGE)
{}

std::string Language::NormalizeCode(const std::string &code) {
    std::string out;
    for (char c : code) {
        if (c == '-')
            c = '_';
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_')
            return "";
        out += c;
    }
    return out;
}

std::optional<Language::StringMap> Language::ReadLanguageFile(const std::string &code) const {
    std::filesystem::path path = mDir / (code + ".luau");
    std::error_code ec;
    if (code.empty() || !std::filesystem::is_regular_file(path, ec))
        return std::nullopt;

    std::ifstream file(path, std::ios::binary);
    if (!file)
        return std::nullopt;
    std::stringstream ss;
    ss << file.rdbuf();

    sol::load_result chunk = mLua->load(ss.str(), "@lang/" + code + ".luau", sol::load_mode::text);
    if (!chunk.valid()) {
        sol::error err = chunk;
        Out("Language", "Failed to compile {}: {}", path.string(), err.what());
        return std::nullopt;
    }

    sol::protected_function fn = chunk;
    sol::environment env(*mLua, sol::create);
    env.set_on(fn);
    sol::protected_function_result res = fn();
    if (!res.valid()) {
        sol::error err = res;
        Out("Language", "Failed to execute {}: {}", path.string(), err.what());
        return std::nullopt;
    }

    sol::object ret = res;
    if (ret.get_type() != sol::type::table) {
        Out("Language", "{} did not return a table", path.string());
        return std::nullopt;
    }

    StringMap strings;
    for (const auto &[key, value] : ret.as<sol::table>()) {
        if (key.get_type() == sol::type::string && value.get_type() == sol::type::string)
            strings[key.as<std::string>()] = value.as<std::string>();
    }
    return strings;
}

bool Language::Load(const std::string &code) {
    std::string normalized = NormalizeCode(code);
    std::optional<StringMap> strings = ReadLanguageFile(normalized);
    bool found = strings.has_value();
    if (!found) {
        Out("Language", "Language \"{}\" not found, using {}", code, NOOBWARRIOR_DEFAULT_LANGUAGE);
        normalized = NOOBWARRIOR_DEFAULT_LANGUAGE;
    }

    mCode = normalized;
    mDefaultStrings = ReadLanguageFile(NOOBWARRIOR_DEFAULT_LANGUAGE).value_or(StringMap {});
    mStrings = mCode == NOOBWARRIOR_DEFAULT_LANGUAGE ? StringMap {} : strings.value_or(StringMap {});
    return found;
}

const std::string& Language::GetCode() const {
    return mCode;
}

std::vector<std::string> Language::GetAvailableLanguages() const {
    std::vector<std::string> codes;
    std::error_code ec;
    for (const auto &entry : std::filesystem::directory_iterator(mDir, ec)) {
        if (entry.is_regular_file(ec) && entry.path().extension() == ".luau")
            codes.push_back(entry.path().stem().string());
    }
    if (std::find(codes.begin(), codes.end(), NOOBWARRIOR_DEFAULT_LANGUAGE) == codes.end())
        codes.push_back(NOOBWARRIOR_DEFAULT_LANGUAGE);
    std::sort(codes.begin(), codes.end());
    return codes;
}

bool Language::Has(const std::string &key) const {
    return mStrings.contains(key) || mDefaultStrings.contains(key);
}

std::string Language::Lookup(const std::string &key, const std::string &fallback) const {
    if (auto it = mStrings.find(key); it != mStrings.end())
        return it->second;
    if (auto it = mDefaultStrings.find(key); it != mDefaultStrings.end())
        return it->second;
    return fallback.empty() ? key : fallback;
}
