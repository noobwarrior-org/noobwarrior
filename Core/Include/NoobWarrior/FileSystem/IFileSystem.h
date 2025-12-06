// === noobWarrior ===
// File: IFileSystem.h
// Started by: Hattozo
// Started on: 12/5/2025
// Description: An abstract interface for a file system.
#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>

namespace NoobWarrior {
struct FSEntry {
    enum class Type {
        File,
        Directory
    };

    bool                    Exists      {};
    bool                    Symlink     {};
    IFileSystem*            SymlinkFs   { nullptr };
    FSEntry::Type           Type;
    uint64_t                Size        {};
    std::string             Name        {};
    std::filesystem::path   RealPath    {};
};

struct FileHandle {

};

class IFileSystem {
public:
    enum class Response {
        Failed,
        Success,
        FileReadFailed,
        InvalidFile
    };

    /**
     * @brief Creates a new virtual file system object based on the type of path given.
     * For example, if a .zip file is passed to this, it will try creating a ZipFileSystem object.
     * If a regular directory is passed, it will create a StdFileSystem object.
     */
    static Response CreateFromFile(IFileSystem** vfsPtr, const std::filesystem::path &path);

    virtual FSEntry GetEntryFromPath(const std::string &path) = 0;
    virtual std::vector<FSEntry> GetEntriesInDirectory(const std::string &path) = 0;

    virtual FileHandle OpenHandle(const std::string &path) = 0;
    virtual void CloseHandle(const FileHandle &handle) = 0;

    /**
     * @brief Reads X amount of bytes from an open file handle. It will automatically seek to the next chunk.
     */
    virtual std::vector<unsigned char> ReadChunk(const FileHandle &handle, unsigned int size) = 0;

    virtual bool EntryExists(const std::string &path) = 0;
    virtual Response DeleteEntry(const std::string &path) = 0;
};
}