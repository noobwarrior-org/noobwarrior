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
#include <NoobWarrior/Lua/LuaState.h>
#include <NoobWarrior/FileSystem/VirtualFileSystem.h>

#include <filesystem>
#include <vector>
#include <string>

namespace NoobWarrior {
class Core;
class Plugin {
public:
    struct Properties {
        std::string Identifier;
        std::string FileName;
        std::string Title;
        std::string Version;
        std::string Description;
        std::string IconFileName;
        std::vector<std::string> Authors;
        bool IsCritical;
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
     *
     * @param includedInInstall A boolean that tells us if this plugin is included in the program's installation directory.
     * This will make it look for the plugin in the installation directory instead of the user data directory.
     * It will also allow it special privileges (like force-enabling it if needed), and it will make it unremovable from the menu
     * unless if manually removed through the computer's file manager.
     */
    Plugin(const std::string &fileName, Core* core, bool includedInInstall = false);
    ~Plugin();

    Response Execute();
    bool Fail();
    Response GetInitResponse();

    std::vector<unsigned char> GetIconData();
    std::string GetFileName();

    const Properties GetProperties();
protected:
    void PushLuaTable();
    Response mResponse { 0 };
private:
    Core* mCore;
    std::string mFileName;
    VirtualFileSystem* mVfs;
    FSEntryHandle mVfsHandle;
    bool mIncludedInInstall;
    int mRef;
};
}