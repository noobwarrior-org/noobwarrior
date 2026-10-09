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
#include <NoobWarrior/FileSystem/VirtualFileSystem.h>
#include <NoobWarrior/Lua/LuaState.h>
#include <NoobWarrior/Log.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <limits>
#include <sstream>

using namespace NoobWarrior;

Language::Language(LuaState* lua, std::filesystem::path dir) :
    mLua(lua),
    mDir(std::move(dir)),
    mCode(NOOBWARRIOR_DEFAULT_LANGUAGE),
    mRequestedCode(NOOBWARRIOR_DEFAULT_LANGUAGE)
{
    mSources.push_back(Source { .Id = "core" });
}

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

std::optional<std::string> Language::ReadSourceFile(const Source &source, const std::string &code) const {
    if (code.empty())
        return std::nullopt;

    if (source.Vfs == nullptr) {
        std::filesystem::path path = mDir / (code + ".luau");
        std::error_code ec;
        if (!std::filesystem::is_regular_file(path, ec))
            return std::nullopt;
        std::ifstream file(path, std::ios::binary);
        if (!file)
            return std::nullopt;
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    std::string path = "/lang/" + code + ".luau";
    if (!source.Vfs->EntryExists(path))
        return std::nullopt;
    FSEntryInfo info = source.Vfs->GetEntryFromPath(path);
    if (info.Failed || info.Type != FSEntryInfo::Type::File || info.Size > std::numeric_limits<unsigned int>::max())
        return std::nullopt;

    FSEntryHandle handle = source.Vfs->OpenHandle(path);
    if (handle == 0)
        return std::nullopt;
    std::vector<unsigned char> bytes;
    bool read = info.Size == 0 || source.Vfs->ReadHandleChunk(handle, &bytes, static_cast<unsigned int>(info.Size));
    source.Vfs->CloseHandle(handle);
    if (!read)
        return std::nullopt;
    return std::string(bytes.begin(), bytes.end());
}

std::vector<std::string> Language::GetSourceCodes(const Source &source) const {
    std::vector<std::string> codes;
    if (source.Vfs == nullptr) {
        std::error_code ec;
        for (const auto &entry : std::filesystem::directory_iterator(mDir, ec)) {
            if (entry.is_regular_file(ec) && entry.path().extension() == ".luau")
                codes.push_back(entry.path().stem().string());
        }
        return codes;
    }

    if (!source.Vfs->EntryExists("/lang"))
        return codes;
    for (const FSEntryInfo &entry : source.Vfs->GetEntriesInDirectory("/lang")) {
        std::filesystem::path name(entry.Name);
        if (!entry.Failed && entry.Type == FSEntryInfo::Type::File && name.extension() == ".luau")
            codes.push_back(name.stem().string());
    }
    return codes;
}

std::optional<Language::StringMap> Language::ReadLanguageFile(const Source &source, const std::string &code) const {
    std::optional<std::string> contents = ReadSourceFile(source, code);
    if (!contents)
        return std::nullopt;

    std::string name = source.Vfs == nullptr
        ? (mDir / (code + ".luau")).string()
        : "plugin://" + source.Id + "/lang/" + code + ".luau";

    sol::load_result chunk = mLua->load(*contents, "@" + name, sol::load_mode::text);
    if (!chunk.valid()) {
        sol::error err = chunk;
        Out("Language", "Failed to compile {}: {}", name, err.what());
        return std::nullopt;
    }

    sol::protected_function fn = chunk;
    sol::environment env(*mLua, sol::create);
    env.set_on(fn);
    sol::protected_function_result res = fn();
    if (!res.valid()) {
        sol::error err = res;
        Out("Language", "Failed to execute {}: {}", name, err.what());
        return std::nullopt;
    }

    sol::object ret = res;
    if (ret.get_type() != sol::type::table) {
        Out("Language", "{} did not return a table", name);
        return std::nullopt;
    }

    StringMap strings;
    for (const auto &[key, value] : ret.as<sol::table>()) {
        if (key.get_type() == sol::type::string && value.get_type() == sol::type::string)
            strings[key.as<std::string>()] = value.as<std::string>();
    }
    return strings;
}

void Language::ReloadSource(Source &source) {
    source.DefaultStrings = ReadLanguageFile(source, NOOBWARRIOR_DEFAULT_LANGUAGE).value_or(StringMap {});
    source.Strings = mCode == NOOBWARRIOR_DEFAULT_LANGUAGE ? StringMap {} : ReadLanguageFile(source, mCode).value_or(StringMap {});
}

bool Language::SourceHasLanguage(const Source &source, const std::string &code) const {
    std::vector<std::string> codes = GetSourceCodes(source);
    return std::find(codes.begin(), codes.end(), code) != codes.end();
}

bool Language::HasLanguage(const std::string &code) const {
    std::vector<std::string> codes = GetAvailableLanguages();
    return std::find(codes.begin(), codes.end(), code) != codes.end();
}

bool Language::Load(const std::string &code) {
    mRequestedCode = NormalizeCode(code);
    bool found = !mRequestedCode.empty() && HasLanguage(mRequestedCode);
    if (!found)
        Out("Language", "Language \"{}\" not found, using {}", code, NOOBWARRIOR_DEFAULT_LANGUAGE);

    mCode = found ? mRequestedCode : NOOBWARRIOR_DEFAULT_LANGUAGE;
    for (Source &source : mSources)
        ReloadSource(source);
    return found;
}

void Language::AddSource(const std::string &id, VirtualFileSystem *vfs) {
    if (vfs == nullptr)
        return;
    std::erase_if(mSources, [&id](const Source &source) { return source.Vfs != nullptr && source.Id == id; });
    mSources.push_back(Source { .Id = id, .Vfs = vfs });

    if (mCode != mRequestedCode && SourceHasLanguage(mSources.back(), mRequestedCode)) {
        Load(mRequestedCode);
        return;
    }
    ReloadSource(mSources.back());
}

void Language::RemoveSource(const std::string &id) {
    size_t removed = std::erase_if(mSources, [&id](const Source &source) { return source.Vfs != nullptr && source.Id == id; });
    if (removed > 0 && !HasLanguage(mCode))
        Load(mRequestedCode);
}

const std::string& Language::GetCode() const {
    return mCode;
}

std::vector<std::string> Language::GetAvailableLanguages() const {
    std::vector<std::string> codes;
    for (const Source &source : mSources) {
        std::vector<std::string> sourceCodes = GetSourceCodes(source);
        codes.insert(codes.end(), sourceCodes.begin(), sourceCodes.end());
    }
    codes.push_back(NOOBWARRIOR_DEFAULT_LANGUAGE);
    std::sort(codes.begin(), codes.end());
    codes.erase(std::unique(codes.begin(), codes.end()), codes.end());
    return codes;
}

bool Language::Has(const std::string &key) const {
    return std::any_of(mSources.begin(), mSources.end(), [&key](const Source &source) {
        return source.Strings.contains(key) || source.DefaultStrings.contains(key);
    });
}

std::string Language::Lookup(const std::string &key, const std::string &fallback) const {
    for (auto source = mSources.rbegin(); source != mSources.rend(); ++source) {
        if (auto it = source->Strings.find(key); it != source->Strings.end())
            return it->second;
    }
    for (auto source = mSources.rbegin(); source != mSources.rend(); ++source) {
        if (auto it = source->DefaultStrings.find(key); it != source->DefaultStrings.end())
            return it->second;
    }
    return fallback.empty() ? key : fallback;
}
