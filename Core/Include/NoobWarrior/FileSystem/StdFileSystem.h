// === noobWarrior ===
// File: StdFileSystem.h
// Started by: Hattozo
// Started on: 12/5/2025
// Description:
#pragma once
#include "VirtualFileSystem.h"

#include <filesystem>
#include <map>
#include <string>
#include <memory>
#include <unordered_map>

namespace NoobWarrior {
class StdFileSystem : public VirtualFileSystem {
public:
    StdFileSystem(const std::filesystem::path &root);
    ~StdFileSystem() override;
    
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
    std::filesystem::path ConstructRealPath(std::string submittedPath);
    std::filesystem::path mRoot;
    std::unordered_map<int, std::shared_ptr<std::fstream>> mHandles;
};
}