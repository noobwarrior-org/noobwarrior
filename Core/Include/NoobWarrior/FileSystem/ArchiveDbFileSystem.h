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
// File: ArchiveDbFileSystem.h
// Started by: Hattozo
// Started on: 10/6/2026
// Description: VirtualFileSystem implementation for .sqlar files.
// It does not fully comply to the .sqlar standard as it uses zstd for compression.
#pragma once
#include "VirtualFileSystem.h"

#include <NoobWarrior/SqlDb/SqlDb.h>

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace NoobWarrior {
class ArchiveDbFileSystem : public VirtualFileSystem {
public:
    static constexpr int kDefaultFileMode = 0100644;
    static constexpr int kDefaultDirectoryMode = 040755;
    static constexpr int kModeTypeMask = 0170000;
    static constexpr int kModeDirectory = 040000;

    ArchiveDbFileSystem(const std::string& path = ":memory:", SqlDb::OpenMode openMode = SqlDb::OpenMode::ReadWrite);
    ~ArchiveDbFileSystem() override;

    std::unique_ptr<VirtualFileSystem> MakeUnique() const override;

    SqlDb* GetDatabase() const { return mDb.get(); }

    void SetCompressionLevel(int level) { mCompressionLevel = level; }
    int GetCompressionLevel() const { return mCompressionLevel; }

    FSEntryInfo GetEntryFromPath(const std::string &path) override;
    std::vector<FSEntryInfo> GetEntriesInDirectory(const std::string &path) override;

    FSEntryHandle OpenHandle(const std::string &path) override;
    Response CloseHandle(FSEntryHandle handle) override;
    bool IsHandleEOF(FSEntryHandle handle) override;
    bool ReadHandleChunk(FSEntryHandle handle, std::vector<unsigned char> *buf, unsigned int size) override;
    bool ReadHandleLine(FSEntryHandle handle, std::string *buf) override;

    bool EntryExists(const std::string &path) override;
    Response DeleteEntry(const std::string &path) override;

    Response WriteFile(const std::string &path, const std::vector<unsigned char> &data) override;
    Response CreateDirectories(const std::string &path) override;

    Response ReadFile(const std::string &path, std::vector<unsigned char> *out);

    static std::string NormalizePath(const std::string &path);

    static Response Compress(const std::vector<unsigned char> &data, std::vector<unsigned char> *out, int level);
    static Response Decompress(const std::vector<unsigned char> &data, int64_t size, std::vector<unsigned char> *out);
protected:
    struct Row {
        std::string Name;
        int Mode { 0 };
        int64_t MTime { 0 };
        int64_t Size { 0 };
    };

    std::optional<Row> GetRow(const std::string &name);
    bool HasChildren(const std::string &name);
    static bool IsDirectoryMode(int mode);
    FSEntryInfo MakeEntry(const std::string &name, bool isDirectory, uint64_t size);

    std::shared_ptr<SqlDb> mDb;
    int mCompressionLevel { 3 };

    struct OpenFile {
        std::vector<unsigned char> Data;
        size_t Cursor { 0 };
    };
    std::map<FSEntryHandle, OpenFile> mHandles;
    FSEntryHandle mNextHandle { 1 };
};
}
