// === noobWarrior ===
// File: StdFileSystem.cpp
// Started by: Hattozo
// Started on: 12/5/2025
// Description:
#include "NoobWarrior/FileSystem/IFileSystem.h"
#include <NoobWarrior/FileSystem/StdFileSystem.h>

#include <cassert>
#include <filesystem>
#include <fstream>

using namespace NoobWarrior;

StdFileSystem::StdFileSystem(const std::filesystem::path &root) : mRoot(root) {
    assert(std::filesystem::is_directory(root) && "Root is not a directory");
}

std::filesystem::path StdFileSystem::ConstructRealPath(std::string submittedPath) {
    while (submittedPath.starts_with('/'))
        submittedPath = submittedPath.substr(1);

    std::filesystem::path resolvedPath = std::filesystem::weakly_canonical(submittedPath);
    std::filesystem::path resolvedRootPath = std::filesystem::weakly_canonical(mRoot);

    if (resolvedRootPath.string().rfind(resolvedPath.string(), 0) == 0) {
        // path has escaped
        return {};
    }

    return resolvedRootPath / resolvedPath;
}

void StdFileSystem::ChangeWorkingDirectory(const std::string &path) {
    
}

FSEntryInfo StdFileSystem::GetEntryFromPath(const std::string &path) {
    std::filesystem::path real_path = ConstructRealPath(path);
    FSEntryInfo entry {};
    entry.Exists = std::filesystem::exists(real_path);
    if (!entry.Exists)
        goto finish;
    entry.Type = std::filesystem::is_directory(real_path) ? FSEntryInfo::Type::Directory : FSEntryInfo::Type::File;
    entry.Size = std::filesystem::is_directory(real_path) ? std::filesystem::file_size(real_path) : 0;
    entry.Name = std::filesystem::path(real_path).filename();
    entry.Path = path;
    entry.RealPath = std::filesystem::absolute(real_path);
finish:
    return entry;
}

std::vector<FSEntryInfo> StdFileSystem::GetEntriesInDirectory(const std::string &path) {
    std::vector<FSEntryInfo> entries;
    std::filesystem::path real_path = ConstructRealPath(path);
    for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator { real_path }) {
        FSEntryInfo entryInfo = GetEntryFromPath(std::filesystem::relative(entry.path(), mRoot).generic_string());
        entries.push_back(entryInfo);
    } 
    return entries;
}

FSEntryHandle StdFileSystem::OpenHandle(const std::string &path) {
    std::filesystem::path real_path = ConstructRealPath(path);
    auto stream = std::make_shared<std::fstream>(real_path);
    if (stream->fail())
        return NULL;
    int id = 0;
    mHandles.emplace(id, std::move(stream));
    return id;
}

bool StdFileSystem::CloseHandle(FSEntryHandle handle) {
    std::shared_ptr<std::fstream> stream = mHandles.at(handle);
    stream->close();
    bool fail = stream->fail();
    mHandles.erase(handle);
    return fail;
}

bool StdFileSystem::IsHandleEOF(FSEntryHandle handle) {
    std::shared_ptr<std::fstream> stream = mHandles.at(handle);
    return stream->eof();
}

bool StdFileSystem::ReadHandleChunk(FSEntryHandle handle, std::vector<unsigned char> *buf, unsigned int size) {
    std::shared_ptr<std::fstream> stream = mHandles.at(handle);
    
    stream->read(*buf, size);
}

bool StdFileSystem::ReadHandleLine(FSEntryHandle handle, std::string *buf) {
    std::shared_ptr<std::fstream> stream = mHandles.at(handle);
}

bool StdFileSystem::EntryExists(const std::string &path) {
    std::filesystem::path real_path = ConstructRealPath(path);
    return std::filesystem::exists(real_path);
}

bool StdFileSystem::DeleteEntry(const std::string &path) {
    std::filesystem::path real_path = ConstructRealPath(path);
    return std::filesystem::remove(real_path);
}
