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
// File: StdFileSystem.cpp
// Started by: Hattozo
// Started on: 12/5/2025
// Description:
#include <NoobWarrior/FileSystem/StdFileSystem.h>
#include <NoobWarrior/FileSystem/VirtualFileSystem.h>
#include <NoobWarrior/Log.h>

#include <cassert>
#include <filesystem>
#include <fstream>

using namespace NoobWarrior;

StdFileSystem::StdFileSystem(const std::filesystem::path &root) : mRoot(root) {
    if (!std::filesystem::is_directory(root))
        mFailCode = 1;
}

StdFileSystem::~StdFileSystem() {

}

std::filesystem::path StdFileSystem::ConstructRealPath(std::string submittedPath) {
    while (submittedPath.starts_with('/'))
        submittedPath = submittedPath.substr(1);
    /*
    if (mRoot.string().rfind(submittedPath, 0) == 0) {
        // path has escaped
        return {};
    }
    */
    return mRoot / submittedPath;
}

std::string StdFileSystem::GetRelativePath(const std::filesystem::path &fullPath) const {
    std::string fullPathStr = fullPath.generic_string();
    std::string rootPath = mRoot.generic_string();

    if (!rootPath.empty() && rootPath.back() != '/')
        rootPath += '/';

    std::string relativePath;
    if (fullPathStr.starts_with(rootPath))
        relativePath = fullPathStr.substr(rootPath.size());
    else
        relativePath = fullPathStr;
    return relativePath;
}

std::unique_ptr<VirtualFileSystem> StdFileSystem::MakeUnique() const {
    return std::make_unique<StdFileSystem>(*this);
}

FSEntryInfo StdFileSystem::GetEntryFromPath(const std::string &path) {
    std::filesystem::path real_path = ConstructRealPath(path);
    std::filesystem::perms permissions;
    FSEntryInfo entry {};
    entry.Owner = this;
    try {
        entry.Exists = std::filesystem::exists(real_path);
        if ((!entry.Exists) || (Fail()))
            goto finish;
        entry.Type = std::filesystem::is_directory(real_path) ? FSEntryInfo::Type::Directory : FSEntryInfo::Type::File;
        entry.Size = !std::filesystem::is_directory(real_path) ? std::filesystem::file_size(real_path) : 0;
        entry.Name = std::filesystem::path(real_path).filename().string();
        entry.Path = path;
        entry.RealPath = std::filesystem::absolute(real_path);
    } catch (std::filesystem::filesystem_error &e) {
        entry.Failed = true;
        entry.Exists = true;
        Out("StdFileSystem", "Warning: Failed to get attributes for entry \"{}\"\nFull Error: \"{}\"", path, e.what());
    }
finish:
    return entry;
}

std::vector<FSEntryInfo> StdFileSystem::GetEntriesInDirectory(const std::string &path) {
    if (Fail())
        return {};

    std::vector<FSEntryInfo> entries;
    std::filesystem::path real_path = ConstructRealPath(path);
    if (std::filesystem::exists(real_path)) {
        for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator { real_path }) {
            FSEntryInfo entryInfo = GetEntryFromPath(GetRelativePath(entry.path()));
            if (!entryInfo.Failed)
                entries.push_back(entryInfo);
        }
    }
    return entries;
}

FSEntryHandle StdFileSystem::OpenHandle(const std::string &path) {
    if (Fail()) {
        Out("StdFileSystem", "Failed to open handle for file \"{}\" because the zip filesystem failed to initialize.", path);
        return 0;
    }

    std::filesystem::path real_path = ConstructRealPath(path);
    auto stream = std::make_shared<std::fstream>(real_path, std::ios::in | std::ios::binary);
    if (stream->fail()) {
        Out("StdFileSystem", "Failed to open handle for file \"{}\"", path);
        return 0;
    }

    int id = 1;
    while (mHandles.contains(id))
        id++;
    mHandles.emplace(id, std::move(stream));
    return id;
}

VirtualFileSystem::Response StdFileSystem::CloseHandle(FSEntryHandle handle) {
    if (Fail())
        return Response::FileSystemFailed;

    if (!mHandles.contains(handle))
        return Response::InvalidHandle;

    std::shared_ptr<std::fstream> stream = mHandles.at(handle);
    stream->close();
    bool fail = stream->fail();
    mHandles.erase(handle);
    return !fail ? Response::Success : Response::Failed;
}

bool StdFileSystem::IsHandleEOF(FSEntryHandle handle) {
    if (Fail() || !mHandles.contains(handle))
        return false;

    std::shared_ptr<std::fstream> stream = mHandles.at(handle);
    return stream->eof();
}

bool StdFileSystem::ReadHandleChunk(FSEntryHandle handle, std::vector<unsigned char> *buf, unsigned int size) {
    if (Fail() || !mHandles.contains(handle))
        return false;

    std::shared_ptr<std::fstream> stream = mHandles.at(handle);

    std::vector<char> buf_c(size);
    stream->read(buf_c.data(), size);

    std::streamsize bytesRead = stream->gcount();
    if (bytesRead == 0) {
        Out("StdFileSystem", "Failed to read chunk for handle ID {}", handle);
        return false;
    }

    buf->assign(buf_c.begin(), buf_c.begin() + bytesRead);
    return !stream->eof();
}

bool StdFileSystem::ReadHandleLine(FSEntryHandle handle, std::string *buf) {
    if (Fail() || !mHandles.contains(handle))
        return false;

    std::shared_ptr<std::fstream> stream = mHandles.at(handle);
    return std::getline(*stream.get(), *buf) ? true : false;
}

bool StdFileSystem::EntryExists(const std::string &path) {
    if (Fail())
        return false;
    std::filesystem::path real_path = ConstructRealPath(path);
    return std::filesystem::exists(real_path);
}

VirtualFileSystem::Response StdFileSystem::DeleteEntry(const std::string &path) {
    if (Fail())
        return Response::FileSystemFailed;
    std::filesystem::path real_path = ConstructRealPath(path);
    return std::filesystem::remove(real_path) ? Response::Success : Response::Failed;
}

VirtualFileSystem::Response StdFileSystem::WriteFile(const std::string &path, const std::vector<unsigned char> &data) {
    if (Fail())
        return Response::FileSystemFailed;

    std::filesystem::path real_path = ConstructRealPath(path);
    std::error_code ec;
    if (real_path.has_parent_path())
        std::filesystem::create_directories(real_path.parent_path(), ec);

    std::ofstream stream(real_path, std::ios::out | std::ios::binary | std::ios::trunc);
    if (stream.fail()) {
        Out("StdFileSystem", "Failed to open file \"{}\" for writing", path);
        return Response::Failed;
    }
    if (!data.empty())
        stream.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    stream.close();
    if (stream.fail()) {
        Out("StdFileSystem", "Failed to write file \"{}\"", path);
        return Response::Failed;
    }
    return Response::Success;
}

VirtualFileSystem::Response StdFileSystem::CreateDirectories(const std::string &path) {
    if (Fail())
        return Response::FileSystemFailed;
    std::filesystem::path real_path = ConstructRealPath(path);
    std::error_code ec;
    std::filesystem::create_directories(real_path, ec);
    return ec ? Response::Failed : Response::Success;
}
