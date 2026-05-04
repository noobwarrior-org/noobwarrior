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
// File: VirtualFileSystem.h
// Started by: Hattozo
// Started on: 12/5/2025
// Description: An abstract interface for a file system.
#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>
#include <memory>

namespace NoobWarrior {
class VirtualFileSystem;
struct FSEntryInfo {
    enum class Type {
        File,
        Directory
    };

    VirtualFileSystem*      Owner       { nullptr };
    bool                    Failed      { false };
    bool                    Exists      {};
    FSEntryInfo::Type       Type        {};
    uint64_t                Size        {};
    std::string             Name        {};
    std::string             Path        {};
    std::filesystem::path   RealPath    {};
};

typedef int FSEntryHandle;

class VirtualFileSystem {
public:
    enum class Response {
        Failed,
        Success,
        FileSystemFailed,
        FileReadFailed,
        InvalidFile,
        InvalidHandle,
        InvalidFileSystem,
        CannotCopyFileSystem,
        NotFound
    };

    enum class Format {
        Invalid,
        Standard,
        Zip,
        Database
    };

    static Format GetFormatFromPath(const std::filesystem::path &path);

    /**
     * @brief Creates a new virtual file system object based on the type of path given.
     * For example, if a .zip file is passed to this, it will try creating a ZipFileSystem object.
     * If a regular directory is passed, it will create a StdFileSystem object.
     */
    static Response New(VirtualFileSystem** vfsPtr, const std::filesystem::path &path);
    static void Free(VirtualFileSystem* vfs);

    virtual ~VirtualFileSystem() = 0;
    inline virtual bool Fail() { return mFailCode != 0; }; // Should return true if the file system failed to initialize

    /*
     * This clones the data of the VFS and stores it inside a smart pointer.
     * Why? Because we assume a VirtualFileSystem is volatile and could get freed at any time, so it is
     * necessary to make sure you can make and manage copies that won't just die.
     *
     * Why not just use std::make_unique? because a VFS is generic and can be one of many derived types, and you are not
     * really supposed to know what derived type it is.
     * std::make_unique requires you to know the exact derived type. that's not very generic, is it?
     */
    virtual std::unique_ptr<VirtualFileSystem> MakeUnique() const = 0;

    virtual FSEntryInfo GetEntryFromPath(const std::string &path) = 0;
    virtual std::vector<FSEntryInfo> GetEntriesInDirectory(const std::string &path) = 0;

    virtual FSEntryHandle OpenHandle(const std::string &path) = 0;
    virtual Response CloseHandle(FSEntryHandle handle) = 0;
    virtual bool IsHandleEOF(FSEntryHandle handle) = 0;

    /**
     * @brief Reads X amount of bytes from an open file handle. It will automatically seek to the next chunk.
     * The buffer is overwritten with the new chunk each time this function is called.
     */
    virtual bool ReadHandleChunk(FSEntryHandle handle, std::vector<unsigned char> *buf, unsigned int size) = 0;

    /**
     * @brief Reads the next upcoming line from a file stream.
     * Note that this does not include a new line at the end of the string, you must include that yourself.
     */
    virtual bool ReadHandleLine(FSEntryHandle handle, std::string *buf) = 0;

    virtual bool EntryExists(const std::string &path) = 0;
    virtual Response DeleteEntry(const std::string &path) = 0;
protected:
    int mFailCode { 0 };
};
}