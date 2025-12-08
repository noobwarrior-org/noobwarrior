// === noobWarrior ===
// File: ZipFileSystem.cpp
// Started by: Hattozo
// Started on: 12/6/2025
// Description:
#include <NoobWarrior/FileSystem/ZipFileSystem.h>
#include <NoobWarrior/FileSystem/IFileSystem.h>
#include <NoobWarrior/NoobWarrior.h>

#include <zip.h>
#include <zipint.h>

using namespace NoobWarrior;

ZipFileSystem::ZipFileSystem(const std::filesystem::path &zipPath) : mArchive(nullptr) {
    int err_code {};

    std::string path_string = zipPath.string();
    mArchive = zip_open(path_string.c_str(), NULL, &err_code);

    zip_error_t error;
    zip_error_init_with_code(&error, err_code);
    Out("ZipFileSystem", "Failed to open zip archive \"{}\": {}", zipPath.filename().string(), zip_error_strerror(&error));
    zip_error_fini(&error);

    mFailCode = err_code;
};

ZipFileSystem::~ZipFileSystem() {
    zip_close(mArchive);
}

FSEntryInfo ZipFileSystem::GetEntryFromPath(const std::string &path) {
    FSEntryInfo entry {};
    int statErr;
    zip_stat_t stat;
    zip_int64_t index = zip_name_locate(mArchive, path.c_str(), 0);
    if (index < 0) {
        entry.Exists = false;
        goto finish;
    }
    statErr = zip_stat_index(mArchive, index, NULL, &stat);
    if (statErr == -1)
        goto finish;
    entry.Type = std::string(stat.name).ends_with('/') ? FSEntryInfo::Type::Directory : FSEntryInfo::Type::File;
    entry.Size = stat.size;
    entry.Name = stat.name;
    entry.Path = path;
    entry.RealPath = path;
finish:
    return entry;
}

std::vector<FSEntryInfo> ZipFileSystem::GetEntriesInDirectory(const std::string &path) {
    if (Fail() || mArchive == nullptr)
        return {};

    std::vector<FSEntryInfo> entries;
    zip_int64_t entries_num = zip_get_num_entries(mArchive, NULL);
    for (int i = 0; i < entries_num; i++) {
        auto entryPath = std::string(zip_get_name(mArchive, i, 0));
        if (entryPath.starts_with(path)) {
            FSEntryInfo entryInfo = GetEntryFromPath(entryPath);
            entries.push_back(entryInfo);
        }
    }
    return entries;
}

FSEntryHandle ZipFileSystem::OpenHandle(const std::string &path) {
    if (Fail() || mArchive == nullptr)
        return NULL;

    zip_file_t *file = zip_fopen(mArchive, path.c_str(), NULL);
    if (file == NULL)
        return NULL;

    int id;
    for (int i = 0; !mHandles.contains(i); i++)
        id = i + 1;
    mHandles.emplace(id, file);
    return id;
}

IFileSystem::Response ZipFileSystem::CloseHandle(FSEntryHandle handle) {
    if (Fail())
        return Response::FileSystemFailed;

    if (!mHandles.contains(handle))
        return Response::InvalidHandle;

    zip_file_t *file = mHandles.at(handle);
    int res = zip_fclose(file);
    mHandles.erase(handle);
    return res == 0 ? Response::Success : Response::Failed;
}

bool ZipFileSystem::IsHandleEOF(FSEntryHandle handle) {
    if (Fail() || !mHandles.contains(handle))
        return false;

    zip_file_t *file = mHandles.at(handle);
    char buf[1];
    int bytes_read = zip_fread(file, buf, 1);
    if (bytes_read > 0)
        zip_fseek(file, -bytes_read, SEEK_CUR); // okay, not eof. go back...
    return bytes_read == 0; // if it's eof, bytes_read should return 0.
}

bool ZipFileSystem::ReadHandleChunk(FSEntryHandle handle, std::vector<unsigned char> *buf, unsigned int size) {
    if (Fail() || !mHandles.contains(handle))
        return false;

    zip_file_t *file = mHandles.at(handle);
    char buf_c[size];
    int bytes_read = zip_fread(file, buf_c, size);
    buf->clear();
    buf->insert(buf->begin(), buf_c, buf_c + size);
    return bytes_read > 0;
}

bool ZipFileSystem::ReadHandleLine(FSEntryHandle handle, std::string *buf) {
    if (Fail() || !mHandles.contains(handle))
        return false;

    zip_file_t *file = mHandles.at(handle);

    buf->clear();
    char buf_c[1];
    while (int bytes_read = zip_fread(file, buf_c, 1) > 0) {
        if (bytes_read < 1) {
            if (bytes_read < 0) buf->clear(); // if its less than 0 (like -1) then that means an error occurred. buffer is shitted. clear it.
            return false; // else it just means we've reached end of file (0 bytes read).
        }
        if (*buf_c == '\n')
            break;
        buf->insert(buf->end(), buf_c, buf_c + 1);
    }
    return IsHandleEOF(handle);
}

bool ZipFileSystem::EntryExists(const std::string &path) {
    zip_int64_t index = zip_name_locate(mArchive, path.c_str(), 0);
    return index != -1;
}

IFileSystem::Response ZipFileSystem::DeleteEntry(const std::string &path) {
    zip_int64_t index = zip_name_locate(mArchive, path.c_str(), 0);
    if (index < 0)
        return Response::InvalidFile;
    return zip_delete(mArchive, index) < 0 ? Response::Success : Response::Failed;
}
