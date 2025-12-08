// === noobWarrior ===
// File: ZipFileSystem.h
// Started by: Hattozo
// Started on: 12/5/2025
// Description:
#pragma once
#include "IFileSystem.h"

#include <zip.h>

#include <filesystem>
#include <map>

namespace NoobWarrior {
class ZipFileSystem : public IFileSystem {
public:
    ZipFileSystem(const std::filesystem::path &zipPath);
    ~ZipFileSystem() override;

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
    zip_t *mArchive;
    std::map<int, zip_file_t*> mHandles;
};
}