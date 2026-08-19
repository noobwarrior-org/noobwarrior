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
// File: XmlRobloxFile.h
// Description: Ported RobloxFiles.XmlFormat.XmlRobloxFile: an XML place/model as an object graph.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/RobloxFile.h>
#include <NoobWarrior/Roblox/FileFormat/Tree/Instance.h>
#include <NoobWarrior/Roblox/FileFormat/XmlFormat/XmlPropertyTokens.h>

#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// Mirrors RobloxFiles.XmlFormat. Distinct from NoobWarrior::Roblox::XmlRobloxFile, which still
// serves the live path, so both can coexist until the swap.
namespace NoobWarrior::Roblox::XmlFormat {

// Shared with XmlRobloxFileReader, which reports the failures the reference throws on
// through the bool/error pair the rest of the port uses.
inline void SetError(std::string *error, std::string message) {
    if (error != nullptr)
        *error = std::move(message);
}

class XmlRobloxFile : public RobloxFile {
public:
    XmlRobloxFile();

    // Referent -> Instance for the document being read, so a Ref property can be resolved
    // against it and a duplicate 'referent' attribute refused (XmlFileReader.cs:157-166). It is
    // threaded through the read instead of being a member because DestroySubtree and
    // CreateObject would otherwise leave it holding dangling and missing entries between loads.
    using ReferentIndex = std::unordered_map<std::string, Instance *>;

    // Shared string key -> its base64 payload, in first-use order. Save rebuilds one of these
    // from the properties it actually writes, the way XmlRobloxFile.cs:140-142 does.
    using SharedStringTable = std::vector<std::pair<std::string, std::string>>;

    int Version {4};
    // Ordered, because entry order is part of the document.
    std::vector<std::pair<std::string, std::string>> Metadata;
    SharedStringTable SharedStrings;
    std::vector<std::unique_ptr<Instance>> Objects;

    std::vector<Instance *> Roots() const;

    bool Load(std::span<const unsigned char> data, std::string *error = nullptr);
    bool Save(std::vector<unsigned char> &output, std::string *error = nullptr) const;

    // RobloxFile
    FileResponse Save(std::vector<unsigned char> &buffer) const override;
    bool AppendLuaSourceContainers(std::span<const LuaSourceContainerSpec> containers,
                                   std::string *error = nullptr) override;

protected:
    FileResponse ReadFile(const std::vector<unsigned char> &buffer) override;

private:
    Instance *CreateInstance(const std::string &className, const std::string &name,
                             Instance *parent) override;
    void DestroySubtree(Instance *instance) override;

private:
    Instance *CreateObject(const std::string &className);
    Instance *FindFirstOfClass(std::string_view className) const;
    void ResolveReferences(const ReferentIndex &instances);

    int mNextReferent {0};
};
}
