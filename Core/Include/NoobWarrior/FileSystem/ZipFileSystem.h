// === noobWarrior ===
// File: ZipFileSystem.h
// Started by: Hattozo
// Started on: 12/5/2025
// Description:
#pragma once
#include "IFileSystem.h"

#include <filesystem>

namespace NoobWarrior {
class ZipFileSystem : public IFileSystem {
public:
    ZipFileSystem();
    ~ZipFileSystem() override;

    Response Open(const std::vector<unsigned char> &zipData);
    Response Open(const std::filesystem::path &zipPath);

    void ChangeWorkingDirectory(const std::string &path = "/") override;

    FSEntryInfo GetEntryFromPath(const std::string &path) override;
    std::vector<FSEntryInfo> GetEntriesInDirectory(const std::string &path) override;

    FSEntryHandle OpenHandle(const std::string &path) override;
    bool CloseHandle(FSEntryHandle handle) override;
    bool IsHandleEOF(FSEntryHandle handle) override;
    bool ReadHandleChunk(FSEntryHandle handle, std::vector<unsigned char> *buf, unsigned int size) override;
    bool ReadHandleLine(FSEntryHandle handle, std::string *buf) override;

    bool EntryExists(const std::string &path) override;
    bool DeleteEntry(const std::string &path) override;
};
}