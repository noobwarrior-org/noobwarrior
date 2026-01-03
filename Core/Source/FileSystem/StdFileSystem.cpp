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
    if (mRoot.string().rfind(submittedPath, 0) == 0) {
        // path has escaped
        return {};
    }
    return mRoot / submittedPath;
}

FSEntryInfo StdFileSystem::GetEntryFromPath(const std::string &path) {
    std::filesystem::path real_path = ConstructRealPath(path);
    FSEntryInfo entry {};
    entry.Exists = std::filesystem::exists(real_path);
    if ((!entry.Exists) || (Fail()))
        goto finish;
    entry.Type = std::filesystem::is_directory(real_path) ? FSEntryInfo::Type::Directory : FSEntryInfo::Type::File;
    entry.Size = std::filesystem::is_directory(real_path) ? std::filesystem::file_size(real_path) : 0;
    entry.Name = std::filesystem::path(real_path).filename().string();
    entry.Path = path;
    entry.RealPath = std::filesystem::absolute(real_path);
finish:
    return entry;
}

std::vector<FSEntryInfo> StdFileSystem::GetEntriesInDirectory(const std::string &path) {
    if (Fail())
        return {};

    std::vector<FSEntryInfo> entries;
    std::filesystem::path real_path = ConstructRealPath(path);
    for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator { real_path }) {
        FSEntryInfo entryInfo = GetEntryFromPath(std::filesystem::relative(entry.path(), mRoot).generic_string());
        entries.push_back(entryInfo);
    } 
    return entries;
}

FSEntryHandle StdFileSystem::OpenHandle(const std::string &path) {
    if (Fail()) {
        Out("StdFileSystem", "Failed to open handle for file \"{}\" because the zip filesystem failed to initialize.", path);
        return NULL;
    }

    std::filesystem::path real_path = ConstructRealPath(path);
    auto stream = std::make_shared<std::fstream>(real_path);
    if (stream->fail()) {
        Out("StdFileSystem", "Failed to open handle for file \"{}\"", path);
        return NULL;
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
    
    char buf_c[size];
    stream->read(buf_c, size);
    buf->clear();
    buf->insert(buf->begin(), buf_c, buf_c + size);
    return !(stream->eof() || stream->fail());
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
