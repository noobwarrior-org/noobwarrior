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
// File: OverlayFileSystem.h
// Started by: Hattozo
// Started on: 12/5/2025
// Description: A pseudo file system that overlays one file system over another.
// OverlayFileSystem requires ownership of each file system that is mounted in order to prevent
// use-after-free's and memory access violations.
#pragma once
#include "VirtualFileSystem.h"

#include <filesystem>
#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace NoobWarrior {
class OverlayFileSystem : public VirtualFileSystem {
public:
    OverlayFileSystem();
    ~OverlayFileSystem() override;

    Response Mount(const std::string &vol, std::unique_ptr<VirtualFileSystem> vfs);
    Response Mount(const std::string &vol, VirtualFileSystem* vfs);
    Response Mount(const std::string &vol, const std::filesystem::path &realPath);
    Response Unmount(VirtualFileSystem* vfs);
    Response Unmount(const std::string &vol);

    std::unique_ptr<VirtualFileSystem> MakeUnique() const override;

    FSEntryInfo GetEntryFromPath(const std::string &path) override;
    std::vector<FSEntryInfo> GetEntriesInDirectory(const std::string &path) override;

    FSEntryHandle OpenHandle(const std::string &path) override;
    Response CloseHandle(FSEntryHandle handle) override;
    bool IsHandleEOF(FSEntryHandle handle) override;

    bool ReadHandleChunk(FSEntryHandle handle, std::vector<unsigned char> *buf, unsigned int size) override;
    bool ReadHandleLine(FSEntryHandle handle, std::string *buf) override;

    bool EntryExists(const std::string &path) override;
    Response DeleteEntry(const std::string &path) override;
protected:
    std::vector<std::pair<VirtualFileSystem*, std::string>> GetVfsCandidates(const std::string &submittedPath);
    std::pair<VirtualFileSystem*, std::string> GetVfsPath(const std::string &submittedPath);
    std::pair<VirtualFileSystem*, int> GetRealHandle(FSEntryHandle handle);
    std::filesystem::path mRoot;
    std::unordered_map<int, std::pair<VirtualFileSystem*, int>> mHandles;
    std::vector<std::pair<std::string, std::unique_ptr<VirtualFileSystem>>> mMountedVolumes;
};
}