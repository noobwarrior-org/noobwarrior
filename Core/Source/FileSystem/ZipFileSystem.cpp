/*
 * Copyright (C) 2026 Hattozo
 *
 * This file is part of noobWarrior.
 *
 * noobWarrior is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * noobWarrior is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with noobWarrior; if not, see
 * <https://www.gnu.org/licenses/>.
 */
// === noobWarrior ===
// File: ZipFileSystem.cpp
// Started by: Hattozo
// Started on: 12/6/2025
// Description:
#include <NoobWarrior/FileSystem/ZipFileSystem.h>
#include <NoobWarrior/FileSystem/VirtualFileSystem.h>
#include <NoobWarrior/NoobWarrior.h>

#include <zip.h>
#include <zipint.h>

using namespace NoobWarrior;

ZipFileSystem::ZipFileSystem(const std::filesystem::path &zipPath) : mArchive(nullptr) {
    int err_code { 0 };

    std::string path_string = zipPath.string();
    mArchive = zip_open(path_string.c_str(), NULL, &err_code);

    if (err_code != 0) {
        zip_error_t error;
        zip_error_init_with_code(&error, err_code);
        Out("ZipFileSystem", "Failed to open zip archive \"{}\": {}", zipPath.filename().string(), zip_error_strerror(&error));
        zip_error_fini(&error);
    }

    mFailCode = err_code;
};

ZipFileSystem::~ZipFileSystem() {
    zip_close(mArchive);
}

FSEntryInfo ZipFileSystem::GetEntryFromPath(const std::string &path) {
    std::string zip_path = path;

    // The zip file format specification specifically says that file paths cannot start with a leading slash.
    // However the virtual file system specifications permit this, so just remove the starting slashes in order to make it happy.
    while (zip_path.starts_with('/'))
        zip_path = zip_path.substr(1);

    FSEntryInfo entry {};
    int statErr;
    zip_stat_t stat;
    zip_int64_t index = zip_name_locate(mArchive, zip_path.c_str(), 0);
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
    if (Fail() || mArchive == nullptr) {
        Out("ZipFileSystem", "Failed to get entries in virtual directory \"{}\" because the zip filesystem failed to initialize.", path);
        return {};
    }

    std::string zip_path = path;
    while (zip_path.starts_with('/'))
        zip_path = zip_path.substr(1);

    std::vector<FSEntryInfo> entries;
    zip_int64_t entries_num = zip_get_num_entries(mArchive, NULL);
    for (int i = 0; i < entries_num; i++) {
        auto entryPath = std::string(zip_get_name(mArchive, i, 0));
        if (entryPath.starts_with(zip_path)) {
            FSEntryInfo entryInfo = GetEntryFromPath(entryPath);
            entries.push_back(entryInfo);
        }
    }
    return entries;
}

FSEntryHandle ZipFileSystem::OpenHandle(const std::string &path) {
    if (Fail() || mArchive == nullptr) {
        Out("ZipFileSystem", "Failed to open handle for file \"{}\" because the zip filesystem failed to initialize.", path);
        return NULL;
    }

    std::string zip_path = path;
    while (zip_path.starts_with('/'))
        zip_path = zip_path.substr(1);

    zip_file_t *file = zip_fopen(mArchive, zip_path.c_str(), NULL);
    if (file == NULL) {
        Out("ZipFileSystem", "Failed to open handle for file \"{}\"", path);
        return NULL;
    }

    int id = 1;
    while (mHandles.contains(id))
        id++;
    mHandles.emplace(id, file);
    return id;
}

VirtualFileSystem::Response ZipFileSystem::CloseHandle(FSEntryHandle handle) {
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
    return bytes_read < 1; // if it's eof, bytes_read should return 0.
}

bool ZipFileSystem::ReadHandleChunk(FSEntryHandle handle, std::vector<unsigned char> *buf, unsigned int size) {
    if (Fail() || !mHandles.contains(handle))
        return false;

    zip_file_t *file = mHandles.at(handle);
    char buf_c[size];
    zip_int64_t bytes_read = zip_fread(file, buf_c, size);
    buf->clear();
    buf->insert(buf->end(), buf_c, buf_c + size);
    return bytes_read > 0;
}

bool ZipFileSystem::ReadHandleLine(FSEntryHandle handle, std::string *buf) {
    if (Fail() || !mHandles.contains(handle))
        return false;

    zip_file_t *file = mHandles.at(handle);

    buf->clear();

    if (IsHandleEOF(handle))
        return false;

    char buf_c[1];
    zip_int64_t bytes_read;
    while (true) {
        bytes_read = zip_fread(file, buf_c, 1);
        if (*buf_c == '\n' || bytes_read < 1)
            break;
        else Out("ZipFileSystem", "Bytes read: {} - Byte: {}", bytes_read, *buf_c);
        buf->insert(buf->end(), buf_c, buf_c + 1);
    }
    return true;
}

bool ZipFileSystem::EntryExists(const std::string &path) {
    std::string zip_path = path;
    while (zip_path.starts_with('/'))
        zip_path = zip_path.substr(1);

    zip_int64_t index = zip_name_locate(mArchive, zip_path.c_str(), 0);
    return index != -1;
}

VirtualFileSystem::Response ZipFileSystem::DeleteEntry(const std::string &path) {
    std::string zip_path = path;
    while (zip_path.starts_with('/'))
        zip_path = zip_path.substr(1);

    zip_int64_t index = zip_name_locate(mArchive, zip_path.c_str(), 0);
    if (index < 0)
        return Response::InvalidFile;
    return zip_delete(mArchive, index) < 0 ? Response::Success : Response::Failed;
}
