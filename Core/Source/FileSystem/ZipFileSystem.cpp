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
    mArchive = zip_open(path_string.c_str(), 0, &err_code);

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

std::unique_ptr<VirtualFileSystem> ZipFileSystem::MakeUnique() const {
    return std::make_unique<ZipFileSystem>(*this);
}

static std::string NormalizeZipPath(const std::string &path) {
    std::string normalized = path;
    // The zip file format specification specifically says that file paths cannot start with a leading slash.
    // However the virtual file system specifications permit this, so just remove the starting slashes in order to make it happy.
    while (normalized.starts_with('/'))
        normalized = normalized.substr(1);
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    return normalized;
}

zip_int64_t ZipFileSystem::LocateEntry(const std::string &path) {
    if (mArchive == nullptr)
        return -1;

    const std::string wanted = NormalizeZipPath(path);
    zip_int64_t index = zip_name_locate(mArchive, wanted.c_str(), 0);
    if (index >= 0)
        return index;

    // Miss: the archive may store backslash separators, so compare normalized names instead.
    zip_int64_t count = zip_get_num_entries(mArchive, 0);
    for (zip_int64_t i = 0; i < count; i++) {
        const char *name = zip_get_name(mArchive, i, 0);
        if (name != nullptr && NormalizeZipPath(name) == wanted)
            return i;
    }
    return -1;
}

FSEntryInfo ZipFileSystem::GetEntryFromPath(const std::string &path) {
    FSEntryInfo entry {};
    entry.Owner = this;
    int statErr;
    zip_stat_t stat;
    zip_int64_t index = LocateEntry(path);
    if (index < 0) {
        entry.Exists = false;
        goto finish;
    }
    statErr = zip_stat_index(mArchive, index, 0, &stat);
    if (statErr == -1)
        goto finish;
    entry.Exists = true;
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

    const std::string zip_path = NormalizeZipPath(path);

    std::vector<FSEntryInfo> entries;
    zip_int64_t entries_num = zip_get_num_entries(mArchive, 0);
    for (int i = 0; i < entries_num; i++) {
        const char *rawName = zip_get_name(mArchive, i, 0);
        if (rawName == nullptr)
            continue;
        std::string entryPath = NormalizeZipPath(rawName);
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
        return 0;
    }

    zip_int64_t index = LocateEntry(path);
    if (index < 0) {
        Out("ZipFileSystem", "Failed to open handle for file \"{}\"", path);
        return 0;
    }

    zip_file_t *file = zip_fopen_index(mArchive, index, 0);
    if (file == 0) {
        Out("ZipFileSystem", "Failed to open handle for file \"{}\"", path);
        return 0;
    }

    // Remember how much of the entry is left so end-of-file can be answered without reading.
    zip_uint64_t remaining = 0;
    zip_stat_t stat;
    zip_stat_init(&stat);
    if (zip_stat_index(mArchive, index, 0, &stat) == 0 && (stat.valid & ZIP_STAT_SIZE))
        remaining = stat.size;

    // TODO: make this not dogshit
    int id = 1;
    while (mHandles.contains(id))
        id++;
    mHandles.emplace(id, file);
    mHandleRemaining[id] = remaining;
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
    mHandleRemaining.erase(handle); // ids get reused, so a stale count would misreport EOF
    return res == 0 ? Response::Success : Response::Failed;
}

bool ZipFileSystem::IsHandleEOF(FSEntryHandle handle) {
    if (Fail() || !mHandles.contains(handle))
        return false;

    // This used to read a byte and rewind with zip_fseek. That silently ate a byte on every call,
    // because zip_fseek only works on stored entries and fails on deflated ones -- which is most of
    // them. Counting down what is left never touches the stream.
    auto it = mHandleRemaining.find(handle);
    return it == mHandleRemaining.end() || it->second == 0;
}

bool ZipFileSystem::ReadHandleChunk(FSEntryHandle handle, std::vector<unsigned char> *buf, unsigned int size) {
    if (Fail() || !mHandles.contains(handle))
        return false;

    zip_file_t *file = mHandles.at(handle);
    buf->clear();
    if (size == 0)
        return true;

    // Heap, not a VLA: callers legitimately ask for chunks far larger than a stack frame.
    buf->resize(size);
    zip_int64_t bytes_read = zip_fread(file, buf->data(), size);
    if (bytes_read < 0) {
        buf->clear();
        return false;
    }
    // Only what was actually read; the tail of a short final read is not data.
    buf->resize(static_cast<std::size_t>(bytes_read));

    auto it = mHandleRemaining.find(handle);
    if (it != mHandleRemaining.end()) {
        zip_uint64_t consumed = static_cast<zip_uint64_t>(bytes_read);
        it->second = consumed >= it->second ? 0 : it->second - consumed;
    }
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
    auto remaining = mHandleRemaining.find(handle);
    while (true) {
        bytes_read = zip_fread(file, buf_c, 1);
        if (bytes_read < 1)
            break;
        if (remaining != mHandleRemaining.end() && remaining->second > 0)
            remaining->second--;
        if (*buf_c == '\n')
            break;
        // A CRLF file would otherwise leave the carriage return at the end of every line, which
        // callers that compare whole lines (the plugin manifest reader) would trip over.
        if (*buf_c != '\r')
            buf->insert(buf->end(), buf_c, buf_c + 1);
    }
    return true;
}

bool ZipFileSystem::EntryExists(const std::string &path) {
    return LocateEntry(path) >= 0;
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
