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
// File: OverlayFileSystem.cpp
// Started by: Hattozo
// Started on: 12/5/2025
// Description: A pseudo file system that overlays one file system over another.
// OverlayFileSystem requires ownership of each file system that is mounted in order to prevent
// use-after-free's and memory access violations.
#include <NoobWarrior/FileSystem/OverlayFileSystem.h>
#include <NoobWarrior/FileSystem/VirtualFileSystem.h>
#include <NoobWarrior/FileSystem/StdFileSystem.h>
#include <NoobWarrior/Log.h>

using namespace NoobWarrior;

OverlayFileSystem::OverlayFileSystem() = default;
OverlayFileSystem::~OverlayFileSystem() = default;

// returns all VFS candidates whose volume prefix matches the submitted path, in mount priority order (most recently mounted first)
// This allows multiple file systems mounted at the same volume (ex: "/") to all be considered,
// with callers falling through to the next candidate if one doesn't have the entry.
std::vector<std::pair<VirtualFileSystem*, std::string>> OverlayFileSystem::GetVfsCandidates(const std::string &submittedPath) {
    std::vector<std::pair<VirtualFileSystem*, std::string>> candidates;
    for (auto &[vol, vfs] : mMountedVolumes) {
        if (submittedPath.starts_with(vol)) {
            std::string relativePath = submittedPath.substr(vol.length());
            candidates.emplace_back(vfs.get(), relativePath);
        }
    }
    return candidates;
}

// convenience wrapper: returns the first matching VFS and its relative path.
// suitable for callers that only need a single result (ex: DeleteEntry)
std::pair<VirtualFileSystem*, std::string> OverlayFileSystem::GetVfsPath(const std::string &submittedPath) {
    auto candidates = GetVfsCandidates(submittedPath);
    if (!candidates.empty())
        return candidates.front();
    return { nullptr, "" };
}

std::pair<VirtualFileSystem*, int> OverlayFileSystem::GetRealHandle(FSEntryHandle handle) {
    auto it = mHandles.find(handle);
    if (it != mHandles.end()) {
        return it->second;
    }
    return { nullptr, 0 };
}

VirtualFileSystem::Response OverlayFileSystem::Mount(const std::string &vol, std::unique_ptr<VirtualFileSystem> vfs) {
    if (vfs == nullptr)
        return Response::InvalidFileSystem;
    if (vfs->Fail())
        return Response::FileSystemFailed;
    mMountedVolumes.emplace(mMountedVolumes.begin(), vol, std::move(vfs));
    return Response::Success;
}

VirtualFileSystem::Response OverlayFileSystem::Mount(const std::string &vol, VirtualFileSystem* vfs) {
    if (vfs == nullptr)
        return Response::InvalidFileSystem;
    if (vfs->Fail())
        return Response::FileSystemFailed;
    // Make a clone of this VFS, because we want to manage the memory ourselves without just taking it away.
    std::unique_ptr<VirtualFileSystem> vfsSmartPtr(vfs->MakeUnique());
    if (vfsSmartPtr == nullptr)
        return Response::CannotCopyFileSystem;
    return Mount(vol, std::move(vfsSmartPtr));
}

VirtualFileSystem::Response OverlayFileSystem::Mount(const std::string &vol, const std::filesystem::path &realPath) {
    auto vfs = std::make_unique<StdFileSystem>(realPath);
    if (vfs == nullptr)
        return Response::InvalidFileSystem;
    if (vfs->Fail())
        return Response::FileSystemFailed;
    return Mount(vol, std::move(vfs));
}

VirtualFileSystem::Response OverlayFileSystem::Unmount(VirtualFileSystem* vfs) {
    auto it = std::find_if(mMountedVolumes.begin(), mMountedVolumes.end(),
        [vfs](const auto &pair) {
            return pair.second.get() == vfs;
        }
    );
    if (it == mMountedVolumes.end())
        return Response::NotFound;
    mMountedVolumes.erase(it);
    return Response::Success;
}

VirtualFileSystem::Response OverlayFileSystem::Unmount(const std::string &vol) {
    auto it = std::find_if(mMountedVolumes.begin(), mMountedVolumes.end(),
        [&vol](const auto &pair) {
            return pair.first == vol;
        }
    );
    if (it == mMountedVolumes.end())
        return Response::NotFound;
    mMountedVolumes.erase(it);
    return Response::Failed;
}

std::unique_ptr<VirtualFileSystem> OverlayFileSystem::MakeUnique() const {
    // OverlayFileSystem's cannot be deep-copied because they store vectors containing un-copyable unique_ptr's inside of them.
    return nullptr;
}

FSEntryInfo OverlayFileSystem::GetEntryFromPath(const std::string &path) {
    for (auto &[vfs, relativePath] : GetVfsCandidates(path)) {
        FSEntryInfo info = vfs->GetEntryFromPath(relativePath);
        if (!info.Name.empty())
            return info;
    }
    return {};
}

std::vector<FSEntryInfo> OverlayFileSystem::GetEntriesInDirectory(const std::string &path) {
    std::vector<FSEntryInfo> merged;
    std::unordered_set<std::string> seen;

    for (auto &[vol, vfs] : mMountedVolumes) {
        if (!path.starts_with(vol))
            continue;
        std::string relativePath = path.substr(vol.size());
        if (!relativePath.empty() && (relativePath.front() == '/' || relativePath.front() == '\\'))
            relativePath = relativePath.substr(1);

        for (auto &entry : vfs->GetEntriesInDirectory(relativePath)) {
            if (seen.insert(entry.Name).second)
                merged.push_back(std::move(entry));
        }
    }
    return merged;
}

FSEntryHandle OverlayFileSystem::OpenHandle(const std::string &path) {
    for (auto &[vfs, relativePath] : GetVfsCandidates(path)) {
        if (!vfs->EntryExists(relativePath))
            continue;
        FSEntryHandle realHandle = vfs->OpenHandle(relativePath);
        if (realHandle != 0) {
            int id = 1;
            while (mHandles.contains(id))
                id++;
            mHandles[id] = { vfs, realHandle };
            return id;
        }
    }
    return 0;
}

VirtualFileSystem::Response OverlayFileSystem::CloseHandle(FSEntryHandle handle) {
    auto [vfs, realHandle] = GetRealHandle(handle);
    if (vfs != nullptr) {
        Response res = vfs->CloseHandle(realHandle);
        if (res == Response::Success)
            mHandles.erase(handle);
        return res;
    }
    return Response::InvalidHandle;
}

bool OverlayFileSystem::IsHandleEOF(FSEntryHandle handle) {
    auto [vfs, realHandle] = GetRealHandle(handle);
    if (vfs != nullptr && realHandle != 0) {
        return vfs->IsHandleEOF(realHandle);
    }
    return false;
}

bool OverlayFileSystem::ReadHandleChunk(FSEntryHandle handle, std::vector<unsigned char> *buf, unsigned int size) {
    auto [vfs, realHandle] = GetRealHandle(handle);
    if (vfs != nullptr && realHandle != 0) {
        return vfs->ReadHandleChunk(realHandle, buf, size);
    }
    return false;
}

bool OverlayFileSystem::ReadHandleLine(FSEntryHandle handle, std::string *buf) {
    auto [vfs, realHandle] = GetRealHandle(handle);
    if (vfs != nullptr && realHandle != 0) {
        return vfs->ReadHandleLine(realHandle, buf);
    }
    return false;
}

bool OverlayFileSystem::EntryExists(const std::string &path) {
    for (auto &[vfs, relativePath] : GetVfsCandidates(path)) {
        if (vfs->EntryExists(relativePath))
            return true;
    }
    return false;
}

VirtualFileSystem::Response OverlayFileSystem::DeleteEntry(const std::string &path) {
    auto [vfs, relativePath] = GetVfsPath(path);
    if (vfs) {
        return vfs->DeleteEntry(relativePath);
    }
    return Response::NotFound;
}