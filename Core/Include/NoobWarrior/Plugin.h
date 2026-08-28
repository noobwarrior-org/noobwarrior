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
// File: Plugin.h
// Started by: Hattozo
// Started on: 12/3/2025
// Description:
#pragma once
#include <NoobWarrior/FileSystem/VirtualFileSystem.h>
#include <NoobWarrior/Log.h>
#include <NoobWarrior/Lua/LuaScript.h>
#include <NoobWarrior/PluginDataModel.h>

#include <sol/sol.hpp>

#include <vector>
#include <string>
#include <cstdint>

namespace NoobWarrior {
enum PluginFlag {
    NW_NON_PRIVILEGED_PLUGINS = 1 << 0,
    NW_PRIVILEGED_PLUGINS = 1 << 1
};

class Core;
class Plugin {
public:
    struct Properties {
        std::filesystem::path FilePath;
        std::string FileName;
        std::string Identifier;
        std::string Title;
        std::string Version;
        std::string Description;
        std::string IconFileName;
        std::vector<unsigned char> IconData;
        std::vector<std::string> Authors;
        bool IsPrivileged;
    };

    /**
     * @brief One entry of the manifest's `databases` array: an EmuDb the plugin ships or points at.
     *
     * A required entry is mounted for as long as the plugin is enabled and is locked in the Database
     * dialog. A non-required entry is only *offered*: it shows up in the dialog's available list and
     * the user opts in, which is then remembered by SourceUrl in databases.mounted.
     */
    struct DeclaredDatabase {
        std::string SourceUrl;        // fully resolved, e.g. plugin://mygame@example.org/databases/x.nwdb
        std::string OwnerIdentifier;  // the declaring plugin's identifier
        bool Required { false };
        // Writable entries are staged into plugindata so the emulator can write to them (a game
        // database needs DataStore and publish writes); everything else mounts read-only, which for
        // a directory plugin means straight off disk with no copy at all.
        bool Writable { false };
    };

    enum class Permission {
        NoSandbox
    };
    
    enum class Response {
        Failed,
        Success,
        FileReadFailed,
        InvalidFile
    };

    /**
     * @brief Constructs a new Plugin object.
     * 
     * @param fileName The name of the file, like "plugin.zip". It can also just be a directory.
     * Do not use an absolute path; this will only check in the user's plugins directory.
     *
     * @param core A pointer to the main noobWarrior instance, so that it can access other services.
     */
    Plugin(const std::filesystem::path &filePath, Core* core);
    ~Plugin();

    Response Execute();
    bool Fail();
    Response GetInitResponse();

    VirtualFileSystem* GetVfs();

    std::vector<unsigned char> GetIconData();
    std::filesystem::path GetFilePath();
    std::string GetFileName();
    std::string GetIdentifier();

    StudioServerBootstrap BuildStudioServerBootstrap(int64_t placeId, int64_t universeId);

    /**
     * @brief Parses the manifest's `databases` array. Malformed entries are logged and skipped rather
     * than failing the plugin, matching how `datamodel` entries are handled.
     */
    std::vector<DeclaredDatabase> GetDeclaredDatabases();

    bool ReadFile(const std::string &path, std::vector<unsigned char> *data);

    /**
     * @brief Copies a file out of the plugin into a real path on disk, streaming it rather than
     * buffering the whole thing, so that a large database does not have to fit in memory. Needed
     * because SQLite cannot read out of a zipped plugin's virtual filesystem.
     */
    bool ExtractFile(const std::string &path, const std::filesystem::path &destination);

    /**
     * @brief True when the plugin is a directory, so its files are real paths SQLite can open.
     */
    bool IsDirectoryBacked();

    const Properties GetProperties();
protected:
    Response mResponse { 0 };
private:
    void OpenEnv();

    Core* mCore;
    std::filesystem::path mFilePath;
    VirtualFileSystem* mVfs;
    FSEntryHandle mVfsHandle;
    bool mIncludedInInstall;

    std::vector<std::unique_ptr<LuaScript>> mScripts;

    sol::table mManifestTbl;
    sol::environment mEnv;
};
}
