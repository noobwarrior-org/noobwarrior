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
     * 
     * @param path 
     * @return IFileSystem* 
     */
    static Response CreateFromFile(IFileSystem** vfsPtr, const std::filesystem::path &path);

    virtual FSEntry GetEntryFromPath(const std::string &path) = 0;
    virtual std::vector<FSEntry> GetEntriesInDirectory(const std::string &path) = 0;
    virtual std::vector<unsigned char> ReadBytes(const std::string &path, unsigned int size) = 0;

    virtual FileHandle OpenHandle(const std::string &path) = 0;

    virtual bool EntryExists(const std::string &path) = 0;
};
}