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
struct FSEntryInfo {
    enum class Type {
        File,
        Directory
    };

    bool                    Exists      {};
    FSEntryInfo::Type       Type        {};
    uint64_t                Size        {};
    std::string             Name        {};
    std::string             Path        {};
    std::filesystem::path   RealPath    {};
};

typedef int FSEntryHandle;

class IFileSystem {
public:
    virtual ~IFileSystem() = 0;
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

    virtual void ChangeWorkingDirectory(const std::string &path = "/") = 0;

    virtual FSEntryInfo GetEntryFromPath(const std::string &path) = 0;
    virtual std::vector<FSEntryInfo> GetEntriesInDirectory(const std::string &path) = 0;

    virtual FSEntryHandle OpenHandle(const std::string &path) = 0;
    virtual bool CloseHandle(FSEntryHandle handle) = 0;
    virtual bool IsHandleEOF(FSEntryHandle handle) = 0;

    /**
     * @brief Reads X amount of bytes from an open file handle. It will automatically seek to the next chunk.
     * The buffer is overwritten with the new chunk each time this function is called.
     */
    virtual bool ReadHandleChunk(FSEntryHandle handle, std::vector<unsigned char> *buf, unsigned int size) = 0;
    virtual bool ReadHandleLine(FSEntryHandle handle, std::string *buf);

    virtual bool EntryExists(const std::string &path) = 0;
    virtual bool DeleteEntry(const std::string &path) = 0;
};
}