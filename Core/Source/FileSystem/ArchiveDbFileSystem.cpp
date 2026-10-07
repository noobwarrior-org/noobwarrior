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
// File: ArchiveDbFileSystem.cpp
// Started by: Hattozo
// Started on: 10/6/2026
// Description: VirtualFileSystem implementation for .sqlar files.
// It does not fully comply to the .sqlar standard as it uses zstd for compression.
#include <NoobWarrior/FileSystem/ArchiveDbFileSystem.h>
#include <NoobWarrior/SqlDb/Statement.h>
#include <NoobWarrior/Log.h>

#include <sqlite3.h>
#include <zlib.h>
#include <zstd.h>

#include <algorithm>
#include <memory>
#include <sstream>

using namespace NoobWarrior;

ArchiveDbFileSystem::ArchiveDbFileSystem(const std::string& path, SqlDb::OpenMode openMode) {
    mDb = std::make_shared<SqlDb>(path, "ArchiveDbFileSystem", openMode);
    if (mDb->Fail()) {
        Out("ArchiveDbFileSystem", "Failed to open archive database \"{}\"", path);
        mFailCode = 1;
        return;
    }

    if (mDb->IsReadOnly()) {
        Statement stmt = mDb->PrepareStatement("SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = 'sqlar';");
        if (stmt.Fail() || stmt.Step() != SQLITE_ROW) {
            Out("ArchiveDbFileSystem", "Archive database \"{}\" has no sqlar table", path);
            mFailCode = 1;
        }
        return;
    }

    if (!mDb->ExecStatement(
        "CREATE TABLE IF NOT EXISTS sqlar("
        "name TEXT PRIMARY KEY, "
        "mode INT, "
        "mtime INT, "
        "sz INT, "
        "data BLOB"
        ");")) {
        Out("ArchiveDbFileSystem", "Failed to create sqlar table in \"{}\": {}", path, mDb->GetLastErrorMsg());
        mFailCode = 1;
    }
}

ArchiveDbFileSystem::~ArchiveDbFileSystem() {}

std::unique_ptr<VirtualFileSystem> ArchiveDbFileSystem::MakeUnique() const {
    return std::make_unique<ArchiveDbFileSystem>(*this);
}

std::string ArchiveDbFileSystem::NormalizePath(const std::string &path) {
    std::string unified = path;
    std::replace(unified.begin(), unified.end(), '\\', '/');

    std::vector<std::string> parts;
    std::string cur;
    std::stringstream ss(unified);
    while (std::getline(ss, cur, '/')) {
        if (cur.empty() || cur == ".")
            continue;
        if (cur == "..") {
            if (!parts.empty())
                parts.pop_back();
            continue;
        }
        parts.push_back(cur);
    }

    std::string out;
    for (const std::string &p : parts) {
        if (!out.empty())
            out += '/';
        out += p;
    }
    return out;
}

bool ArchiveDbFileSystem::IsDirectoryMode(int mode) {
    return (mode & kModeTypeMask) == kModeDirectory;
}

VirtualFileSystem::Response ArchiveDbFileSystem::Compress(const std::vector<unsigned char> &data, std::vector<unsigned char> *out, int level) {
    if (out == nullptr)
        return Response::Failed;

    if (data.empty()) {
        out->clear();
        return Response::Success;
    }

    std::vector<unsigned char> compressed(ZSTD_compressBound(data.size()));
    size_t size = ZSTD_compress(compressed.data(), compressed.size(), data.data(), data.size(), level);
    if (ZSTD_isError(size)) {
        Out("ArchiveDbFileSystem", "ZSTD compression failed: {}", ZSTD_getErrorName(size));
        return Response::Failed;
    }

    if (size >= data.size()) {
        *out = data;
        return Response::Success;
    }

    compressed.resize(size);
    *out = std::move(compressed);
    return Response::Success;
}

VirtualFileSystem::Response ArchiveDbFileSystem::Decompress(const std::vector<unsigned char> &data, int64_t size, std::vector<unsigned char> *out) {
    if (out == nullptr)
        return Response::Failed;

    if (size < 0 || static_cast<uint64_t>(size) == data.size()) {
        *out = data;
        return Response::Success;
    }

    out->assign(static_cast<size_t>(size), 0);

    if (data.size() >= 4 && data[0] == 0x28 && data[1] == 0xB5 && data[2] == 0x2F && data[3] == 0xFD) {
        unsigned long long contentSize = ZSTD_getFrameContentSize(data.data(), data.size());
        if (contentSize != ZSTD_CONTENTSIZE_ERROR && contentSize != ZSTD_CONTENTSIZE_UNKNOWN && contentSize != static_cast<unsigned long long>(size)) {
            out->clear();
            return Response::FileReadFailed;
        }
        size_t result = ZSTD_decompress(out->data(), out->size(), data.data(), data.size());
        if (ZSTD_isError(result) || result != out->size()) {
            Out("ArchiveDbFileSystem", "ZSTD decompression failed: {}", ZSTD_isError(result) ? ZSTD_getErrorName(result) : "size mismatch");
            out->clear();
            return Response::FileReadFailed;
        }
        return Response::Success;
    }

    uLongf destLen = static_cast<uLongf>(out->size());
    int res = uncompress(out->data(), &destLen, data.data(), static_cast<uLong>(data.size()));
    if (res != Z_OK || destLen != out->size()) {
        Out("ArchiveDbFileSystem", "zlib decompression failed with code {}", res);
        out->clear();
        return Response::FileReadFailed;
    }
    return Response::Success;
}

std::optional<ArchiveDbFileSystem::Row> ArchiveDbFileSystem::GetRow(const std::string &name) {
    if (Fail() || name.empty())
        return std::nullopt;

    Statement stmt = mDb->PrepareStatement("SELECT name, mode, mtime, sz FROM sqlar WHERE name = ?;");
    if (stmt.Fail())
        return std::nullopt;
    stmt.Bind(1, name);

    if (stmt.Step() != SQLITE_ROW)
        return std::nullopt;

    Row row;
    row.Name = stmt.GetStringFromColumnIndex(0);
    row.Mode = stmt.GetIntFromColumnIndex(1);
    row.MTime = stmt.GetInt64FromColumnIndex(2);
    row.Size = stmt.GetInt64FromColumnIndex(3);
    return row;
}

bool ArchiveDbFileSystem::HasChildren(const std::string &name) {
    if (Fail())
        return false;
    if (name.empty()) {
        Statement stmt = mDb->PrepareStatement("SELECT 1 FROM sqlar LIMIT 1;");
        return !stmt.Fail() && stmt.Step() == SQLITE_ROW;
    }

    Statement stmt = mDb->PrepareStatement("SELECT 1 FROM sqlar WHERE substr(name, 1, length(?1)) = ?1 LIMIT 1;");
    if (stmt.Fail())
        return false;
    stmt.Bind(1, name + "/");
    return stmt.Step() == SQLITE_ROW;
}

FSEntryInfo ArchiveDbFileSystem::MakeEntry(const std::string &name, bool isDirectory, uint64_t size) {
    FSEntryInfo entry {};
    entry.Owner = this;
    entry.Exists = true;
    entry.Type = isDirectory ? FSEntryInfo::Type::Directory : FSEntryInfo::Type::File;
    entry.Size = isDirectory ? 0 : size;
    size_t slash = name.find_last_of('/');
    entry.Name = slash == std::string::npos ? name : name.substr(slash + 1);
    entry.Path = "/" + name;
    return entry;
}

FSEntryInfo ArchiveDbFileSystem::GetEntryFromPath(const std::string &path) {
    std::string name = NormalizePath(path);

    FSEntryInfo entry {};
    entry.Owner = this;
    entry.Path = "/" + name;

    if (Fail()) {
        entry.Failed = true;
        return entry;
    }

    if (name.empty())
        return MakeEntry(name, true, 0);

    if (std::optional<Row> row = GetRow(name))
        return MakeEntry(name, IsDirectoryMode(row->Mode), row->Size < 0 ? 0 : static_cast<uint64_t>(row->Size));

    if (HasChildren(name))
        return MakeEntry(name, true, 0);

    entry.Exists = false;
    return entry;
}

std::vector<FSEntryInfo> ArchiveDbFileSystem::GetEntriesInDirectory(const std::string &path) {
    if (Fail()) {
        Out("ArchiveDbFileSystem", "Failed to get entries in virtual directory \"{}\" because the archive filesystem failed to initialize.", path);
        return {};
    }

    std::string name = NormalizePath(path);
    std::string prefix = name.empty() ? "" : name + "/";

    Statement stmt = mDb->PrepareStatement(
        "SELECT name, mode, sz FROM sqlar WHERE substr(name, 1, length(?1)) = ?1 ORDER BY name;");
    if (stmt.Fail())
        return {};
    stmt.Bind(1, prefix);

    std::map<std::string, FSEntryInfo> children;
    while (stmt.Step() == SQLITE_ROW) {
        std::string rowName = NormalizePath(stmt.GetStringFromColumnIndex(0));
        if (rowName.size() <= prefix.size() || !rowName.starts_with(prefix))
            continue;

        std::string rest = rowName.substr(prefix.size());
        size_t slash = rest.find('/');
        if (slash != std::string::npos) {
            std::string childName = prefix + rest.substr(0, slash);
            if (!children.contains(childName))
                children.emplace(childName, MakeEntry(childName, true, 0));
            continue;
        }

        int mode = stmt.GetIntFromColumnIndex(1);
        int64_t size = stmt.GetInt64FromColumnIndex(2);
        children.insert_or_assign(rowName, MakeEntry(rowName, IsDirectoryMode(mode), size < 0 ? 0 : static_cast<uint64_t>(size)));
    }

    std::vector<FSEntryInfo> entries;
    entries.reserve(children.size());
    for (auto &[_, entry] : children)
        entries.push_back(std::move(entry));
    return entries;
}

VirtualFileSystem::Response ArchiveDbFileSystem::ReadFile(const std::string &path, std::vector<unsigned char> *out) {
    if (Fail())
        return Response::FileSystemFailed;
    if (out == nullptr)
        return Response::Failed;
    out->clear();

    std::string name = NormalizePath(path);
    if (name.empty())
        return Response::InvalidFile;

    Statement stmt = mDb->PrepareStatement("SELECT mode, sz, data FROM sqlar WHERE name = ?;");
    if (stmt.Fail())
        return Response::Failed;
    stmt.Bind(1, name);

    if (stmt.Step() != SQLITE_ROW)
        return Response::NotFound;
    if (IsDirectoryMode(stmt.GetIntFromColumnIndex(0)))
        return Response::InvalidFile;

    int64_t size = stmt.GetInt64FromColumnIndex(1);
    if (stmt.IsColumnIndexNull(2))
        return size <= 0 ? Response::Success : Response::FileReadFailed;

    return Decompress(stmt.GetBlobFromColumnIndex(2), size, out);
}

FSEntryHandle ArchiveDbFileSystem::OpenHandle(const std::string &path) {
    if (Fail()) {
        Out("ArchiveDbFileSystem", "Failed to open handle for file \"{}\" because the archive filesystem failed to initialize.", path);
        return 0;
    }

    OpenFile file;
    if (ReadFile(path, &file.Data) != Response::Success) {
        Out("ArchiveDbFileSystem", "Failed to open handle for file \"{}\"", path);
        return 0;
    }

    while (mNextHandle == 0 || mHandles.contains(mNextHandle))
        mNextHandle++;
    FSEntryHandle id = mNextHandle++;
    mHandles.emplace(id, std::move(file));
    return id;
}

VirtualFileSystem::Response ArchiveDbFileSystem::CloseHandle(FSEntryHandle handle) {
    auto it = mHandles.find(handle);
    if (it == mHandles.end())
        return Response::InvalidHandle;
    mHandles.erase(it);
    return Response::Success;
}

bool ArchiveDbFileSystem::IsHandleEOF(FSEntryHandle handle) {
    auto it = mHandles.find(handle);
    if (it == mHandles.end())
        return false;
    return it->second.Cursor >= it->second.Data.size();
}

bool ArchiveDbFileSystem::ReadHandleChunk(FSEntryHandle handle, std::vector<unsigned char> *buf, unsigned int size) {
    auto it = mHandles.find(handle);
    if (it == mHandles.end() || buf == nullptr)
        return false;

    buf->clear();
    if (size == 0)
        return true;

    OpenFile &file = it->second;
    if (file.Cursor >= file.Data.size())
        return false;

    size_t take = std::min(static_cast<size_t>(size), file.Data.size() - file.Cursor);
    buf->assign(file.Data.begin() + file.Cursor, file.Data.begin() + file.Cursor + take);
    file.Cursor += take;
    return true;
}

bool ArchiveDbFileSystem::ReadHandleLine(FSEntryHandle handle, std::string *buf) {
    auto it = mHandles.find(handle);
    if (it == mHandles.end() || buf == nullptr)
        return false;

    buf->clear();
    OpenFile &file = it->second;
    if (file.Cursor >= file.Data.size())
        return false;

    while (file.Cursor < file.Data.size()) {
        char c = static_cast<char>(file.Data[file.Cursor++]);
        if (c == '\n')
            break;
        if (c != '\r')
            buf->push_back(c);
    }
    return true;
}

bool ArchiveDbFileSystem::EntryExists(const std::string &path) {
    if (Fail())
        return false;
    std::string name = NormalizePath(path);
    if (name.empty())
        return true;
    return GetRow(name).has_value() || HasChildren(name);
}

VirtualFileSystem::Response ArchiveDbFileSystem::DeleteEntry(const std::string &path) {
    if (Fail())
        return Response::FileSystemFailed;

    std::string name = NormalizePath(path);
    if (name.empty())
        return Response::Failed;

    Statement stmt = mDb->PrepareStatement(
        "DELETE FROM sqlar WHERE name = ?1 OR substr(name, 1, length(?2)) = ?2;");
    if (stmt.Fail())
        return Response::Failed;
    stmt.Bind(1, name);
    stmt.Bind(2, name + "/");

    if (stmt.Step() != SQLITE_DONE)
        return Response::Failed;
    return sqlite3_changes(mDb->Get()) > 0 ? Response::Success : Response::NotFound;
}

VirtualFileSystem::Response ArchiveDbFileSystem::WriteFile(const std::string &path, const std::vector<unsigned char> &data) {
    if (Fail())
        return Response::FileSystemFailed;
    if (mDb->IsReadOnly())
        return Response::Failed;

    std::string name = NormalizePath(path);
    if (name.empty())
        return Response::Failed;

    if (std::optional<Row> existing = GetRow(name); existing && IsDirectoryMode(existing->Mode))
        return Response::Failed;
    if (HasChildren(name))
        return Response::Failed;

    size_t slash = name.find_last_of('/');
    if (slash != std::string::npos) {
        Response dirRes = CreateDirectories(name.substr(0, slash));
        if (dirRes != Response::Success)
            return dirRes;
    }

    std::vector<unsigned char> stored;
    Response compressRes = Compress(data, &stored, mCompressionLevel);
    if (compressRes != Response::Success)
        return compressRes;

    Statement stmt = mDb->PrepareStatement(
        "INSERT INTO sqlar (name, mode, mtime, sz, data) VALUES (?, ?, unixepoch(), ?, ?) "
        "ON CONFLICT(name) DO UPDATE SET mtime = excluded.mtime, sz = excluded.sz, data = excluded.data;");
    if (stmt.Fail())
        return Response::Failed;
    stmt.Bind(1, name);
    stmt.Bind(2, kDefaultFileMode);
    stmt.Bind(3, static_cast<int64_t>(data.size()));
    stmt.Bind(4, stored);

    return stmt.Step() == SQLITE_DONE ? Response::Success : Response::Failed;
}

VirtualFileSystem::Response ArchiveDbFileSystem::CreateDirectories(const std::string &path) {
    if (Fail())
        return Response::FileSystemFailed;

    std::string name = NormalizePath(path);
    if (name.empty())
        return Response::Success;
    if (mDb->IsReadOnly())
        return Response::Failed;

    size_t pos = 0;
    while (true) {
        size_t slash = name.find('/', pos);
        std::string current = slash == std::string::npos ? name : name.substr(0, slash);

        std::optional<Row> row = GetRow(current);
        if (row) {
            if (!IsDirectoryMode(row->Mode))
                return Response::Failed;
        } else {
            Statement stmt = mDb->PrepareStatement(
                "INSERT INTO sqlar (name, mode, mtime, sz, data) VALUES (?, ?, unixepoch(), 0, NULL);");
            if (stmt.Fail())
                return Response::Failed;
            stmt.Bind(1, current);
            stmt.Bind(2, kDefaultDirectoryMode);
            if (stmt.Step() != SQLITE_DONE)
                return Response::Failed;
        }

        if (slash == std::string::npos)
            break;
        pos = slash + 1;
    }
    return Response::Success;
}
