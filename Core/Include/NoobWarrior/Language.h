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
// File: Language.h
// Started by: Hattozo
// Started on: 10/9/2026
// Description: Translates keys into text for the selected language
#pragma once
#include <filesystem>
#include <format>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#define NOOBWARRIOR_DEFAULT_LANGUAGE "en_us"

namespace NoobWarrior {
class LuaState;
class VirtualFileSystem;

class Language {
public:
    using StringMap = std::unordered_map<std::string, std::string>;

    Language(LuaState* lua, std::filesystem::path dir);

    bool Load(const std::string &code);
    void AddSource(const std::string &id, VirtualFileSystem *vfs);
    void RemoveSource(const std::string &id);
    const std::string& GetCode() const;
    std::vector<std::string> GetAvailableLanguages() const;

    bool Has(const std::string &key) const;
    std::string Lookup(const std::string &key, const std::string &fallback = "") const;

    template <typename... Args>
    std::string Translate(const std::string &key, const std::string &fallback, const Args&... args) const {
        std::string pattern = Lookup(key, fallback);
        try {
            return std::vformat(pattern, std::make_format_args(args...));
        } catch (const std::format_error &) {
            return pattern;
        }
    }

    static std::string NormalizeCode(const std::string &code);
private:
    struct Source {
        std::string         Id;
        VirtualFileSystem*  Vfs { nullptr };
        StringMap           Strings;
        StringMap           DefaultStrings;
    };

    std::optional<std::string> ReadSourceFile(const Source &source, const std::string &code) const;
    std::vector<std::string> GetSourceCodes(const Source &source) const;
    std::optional<StringMap> ReadLanguageFile(const Source &source, const std::string &code) const;
    void ReloadSource(Source &source);
    bool HasLanguage(const std::string &code) const;
    bool SourceHasLanguage(const Source &source, const std::string &code) const;

    LuaState*               mLua;
    std::filesystem::path   mDir;
    std::string             mCode;
    std::string             mRequestedCode;
    std::vector<Source>     mSources;
};
}
