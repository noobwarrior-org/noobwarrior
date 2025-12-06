// === noobWarrior ===
// File: StdFileSystem.h
// Started by: Hattozo
// Started on: 12/5/2025
// Description:
#pragma once
#include "IFileSystem.h"

#include <filesystem>

namespace NoobWarrior {
class StdFileSystem : public IFileSystem {
public:
    StdFileSystem(const std::filesystem::path &root);
    
    FSEntry GetEntryFromPath(const std::string &path) override;
    std::vector<FSEntry> GetEntriesInDirectory(const std::string &path) override;

    FileHandle OpenHandle(const std::string &path) override;
    void CloseHandle(const FileHandle &handle) override;

    std::vector<unsigned char> ReadChunk(const FileHandle &handle, unsigned int size) override;

    bool EntryExists(const std::string &path) override;
    Response DeleteEntry(const std::string &path) override;
};
}