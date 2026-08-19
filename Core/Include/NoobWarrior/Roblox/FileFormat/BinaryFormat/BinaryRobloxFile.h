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
// File: BinaryRobloxFile.h
// Started by: Hattozo
// Started on: 8/18/2025
// Description: Ported RobloxFiles.BinaryFormat.BinaryRobloxFile: chunks decoded into an object graph.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryFileChunk.h>
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/Chunks/INST.h>
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/Chunks/META.h>
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/Chunks/PRNT.h>
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/Chunks/PROP.h>
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/Chunks/SIGN.h>
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/Chunks/SSTR.h>
#include <NoobWarrior/Roblox/FileFormat/RobloxFile.h>
#include <NoobWarrior/Roblox/FileFormat/Tree/Instance.h>

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

// Mirrors RobloxFiles.BinaryFormat. The namespace keeps this distinct from the existing
// NoobWarrior::Roblox::BinaryRobloxFile, which still serves the live chunk-surgery path, so both
// can coexist until Save is proven and the old one is retired.
namespace NoobWarrior::Roblox::BinaryFormat {

// Full document model: chunks are decoded into an object graph on Load and rebuilt from that graph
// on Save, rather than mutated in place.
// One step of a DataModel path, e.g. {"ServerScriptService", "ServerScriptService"}. This has
// no counterpart in RobloxFiles; it exists so plugin models can be planted at a known location.
struct BinaryModelPathElement {
    std::string Name;
    std::string ClassName;
};

class BinaryRobloxFile : public RobloxFile {
public:
    BinaryRobloxFile();

    uint16_t Version {};
    uint32_t NumClasses {};
    uint32_t NumObjects {};
    uint64_t Reserved {};

    // Decoded chunks, kept as RobloxFiles keeps them: useful for inspection and for
    // re-encoding checks that need the original chunk bodies.
    std::vector<BinaryRobloxFileChunk> Chunks;
    std::vector<INST> Classes;
    // Indexed by referent. Entries can be null when a file skips a referent.
    std::vector<std::unique_ptr<Instance>> Objects;

    SSTR SharedStrings;
    META Metadata;
    SIGN Signatures;
    bool HasSharedStrings {};
    bool HasMetadata {};
    bool HasSignatures {};

    // Roots are the objects whose parent referent is -1.
    std::vector<Instance *> Roots() const;
    Instance *GetObject(int32_t referent) const;
    // Classes are a vector here rather than a map, so lookup by the id stored in a
    // PROP chunk goes through this.
    const INST *FindClass(int32_t classIndex) const;

    // Tree editing used by plugin mounting. CreateInstance allocates a referent and attaches a
    // Name property; DestroySubtree removes an instance and everything beneath it.
    Instance *CreateInstance(const std::string &className, const std::string &name,
                             Instance *parent) override;
    void DestroySubtree(Instance *instance) override;

    bool Load(std::span<const unsigned char> data, std::string *error = nullptr);
    bool Save(std::vector<unsigned char> &output, std::string *error = nullptr) const;

    // RobloxFile
    FileResponse Save(std::vector<unsigned char> &buffer) const override;
    bool AppendLuaSourceContainers(std::span<const LuaSourceContainerSpec> containers,
                                   std::string *error = nullptr) override;

    // Merges every root of an RBXM into this document beneath parentPath, allocating fresh
    // referents and rewriting Ref, Content and SharedString values to match.
    bool AppendBinaryModel(std::span<const unsigned char> model,
                           std::span<const BinaryModelPathElement> parentPath,
                           std::string_view singleRootName = {},
                           std::string *error = nullptr,
                           size_t *replacedRoots = nullptr);

protected:
    FileResponse ReadFile(const std::vector<unsigned char> &buffer) override;

private:
    // Groups objects by ClassName, exactly as RobloxFiles does, so a class name can never appear
    // in two INST chunks. Parents come from the live tree, so a mutated graph serializes correctly
    // without a side table to keep in step.
    std::vector<INST> BuildTables() const;
    // Objects below this referent came from the file; anything above it was mounted in.
    // Closes the gaps DestroySubtree leaves in Objects and rewrites every referent that
    // pointed across one. Serializing calls this because the format has no way to express a
    // hole: an object's referent is its index, so the list has to be dense.
    void CompactObjects();

    size_t mLoadedObjectCount {};
    Instance *CreateObject(const std::string &className);
    Instance *FindFirstOfClass(std::string_view className) const;
    Instance *ResolvePath(std::span<const BinaryModelPathElement> parentPath);
};
}
