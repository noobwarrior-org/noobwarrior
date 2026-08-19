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
// File: RobloxFile.h
// Started by: Hattozo
// Started on: 3/9/2025
// Description:
#pragma once

#include "Tree/Instance.h"

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace NoobWarrior::Roblox {
enum class FileResponse {
    Failed,
    Success,
    InvalidHeader,
    CouldNotParse,
    InvalidVersion,
    VersionTooLow
};

struct LuaSourceContainerSpec {
    std::string_view ClassName;
    std::string_view Name;
    std::string_view Source;
    bool Disabled {};
    std::string_view ParentClassName {"ServerScriptService"};
};

class RobloxFile : public Instance {
public:
    static bool LogErrors;

    virtual ~RobloxFile() = default;

    static FileResponse Open(RobloxFile **file, std::vector<unsigned char> buffer);
    static FileResponse Open(std::unique_ptr<RobloxFile> &file,
                             const std::vector<unsigned char> &buffer);
    static FileResponse Open(RobloxFile **file, std::string_view filePath);
    static FileResponse Open(std::unique_ptr<RobloxFile> &file, std::string_view filePath);

    virtual FileResponse Save(std::vector<unsigned char> &buffer) const = 0;
    virtual bool AppendLuaSourceContainer(std::string_view className,
                                          std::string_view name,
                                          std::string_view source,
                                          bool disabled,
                                          std::string *error = nullptr,
                                          std::string_view parentClassName =
                                              "ServerScriptService");
    virtual bool AppendLuaSourceContainers(
        std::span<const LuaSourceContainerSpec> containers,
        std::string *error = nullptr) = 0;

    // Tree editing, so callers can mount content without caring whether the place is the
    // binary or the XML format. Save() stays virtual, so each keeps its own encoding.
    virtual Instance *CreateInstance(const std::string &className, const std::string &name,
                                     Instance *parent) = 0;
    virtual void DestroySubtree(Instance *instance) = 0;
    FileResponse Save(std::string_view filePath) const;

    const std::string &GetLastError() const;
protected:
    void SetLastError(std::string error);
    virtual FileResponse ReadFile(const std::vector<unsigned char> &buffer) = 0;

private:
    std::string mLastError;
};
}
