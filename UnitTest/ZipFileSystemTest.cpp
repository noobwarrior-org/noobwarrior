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
// File: ZipFileSystemTest.cpp
// Started by: Hattozo
// Started on: 8/27/2026
// Description: Reading out of a zipped plugin. Every case here is a bug that made zipped plugins
// unusable: entries are deflated (so they cannot be seeked), Windows' Compress-Archive writes
// backslash separators, and a chunk read must report only the bytes it actually got.
#include <NoobWarrior/FileSystem/ZipFileSystem.h>

#include <gtest/gtest.h>
#include <zip.h>

#include <cstring>
#include <deque>
#include <filesystem>
#include <string>
#include <vector>

using namespace NoobWarrior;

namespace {
// Builds a real deflated archive; the compression is the point, since a stored entry would hide the
// seek bug entirely.
class TempZip {
public:
    TempZip() {
        // Named after the running test: a shared path would let one test open the archive another
        // one left behind.
        const ::testing::TestInfo *info = ::testing::UnitTest::GetInstance()->current_test_info();
        mPath = std::filesystem::temp_directory_path() /
            ("nw_ziptest_" + std::string(info != nullptr ? info->name() : "unknown") + ".zip");
        std::filesystem::remove(mPath);
    }
    ~TempZip() {
        std::error_code ec;
        std::filesystem::remove(mPath, ec);
    }

    // name is stored verbatim, so a test can reproduce a backslash-separated archive.
    bool Add(const std::string &name, const std::string &contents) {
        int err = 0;
        zip_t *archive = zip_open(mPath.string().c_str(), ZIP_CREATE, &err);
        if (archive == nullptr)
            return false;
        mBlobs.push_back(contents);
        const std::string &stored = mBlobs.back();
        zip_source_t *source = zip_source_buffer(archive, stored.data(), stored.size(), 0);
        if (source == nullptr) {
            zip_discard(archive);
            return false;
        }
        zip_int64_t index = zip_file_add(archive, name.c_str(), source, ZIP_FL_OVERWRITE);
        if (index < 0) {
            zip_source_free(source);
            zip_discard(archive);
            return false;
        }
        zip_set_file_compression(archive, static_cast<zip_uint64_t>(index), ZIP_CM_DEFLATE, 9);
        return zip_close(archive) == 0;
    }

    const std::filesystem::path &Path() const { return mPath; }
private:
    std::filesystem::path mPath;
    std::deque<std::string> mBlobs; // must outlive zip_close; deque never reallocates its elements
};

std::string ReadAllLines(ZipFileSystem &fs, const std::string &path) {
    FSEntryHandle handle = fs.OpenHandle(path);
    if (handle == 0)
        return "<no handle>";
    std::string all, line;
    while (fs.ReadHandleLine(handle, &line))
        all += line + "\n";
    fs.CloseHandle(handle);
    return all;
}
} // namespace

TEST(ZipFileSystem, ReadsEveryByteOfADeflatedEntry) {
    // The old EOF probe read a byte and rewound with zip_fseek, which silently fails on a deflated
    // entry, so the first character of every line was eaten.
    TempZip zip;
    const std::string contents = "return {\n    identifier = \"x@y.z\",\n    title = \"X\"\n}\n";
    ASSERT_TRUE(zip.Add("plugin.lua", contents));

    ZipFileSystem fs(zip.Path());
    ASSERT_FALSE(fs.Fail());
    EXPECT_EQ(contents, ReadAllLines(fs, "/plugin.lua"));
}

TEST(ZipFileSystem, FindsEntriesStoredWithBackslashSeparators) {
    // Windows' Compress-Archive writes "dir\file" even though the spec mandates "dir/file".
    TempZip zip;
    ASSERT_TRUE(zip.Add("plugin.lua", "return {}\n"));
    ASSERT_TRUE(zip.Add("databases\\content.nwdb", "not really a database"));

    ZipFileSystem fs(zip.Path());
    ASSERT_FALSE(fs.Fail());

    EXPECT_TRUE(fs.EntryExists("/plugin.lua"));
    EXPECT_TRUE(fs.EntryExists("/databases/content.nwdb"))
        << "a backslash-separated entry must still be reachable by its virtual path";
    EXPECT_FALSE(fs.EntryExists("/databases/nope.nwdb"));

    FSEntryInfo info = fs.GetEntryFromPath("/databases/content.nwdb");
    EXPECT_TRUE(info.Exists);
    EXPECT_EQ(FSEntryInfo::Type::File, info.Type);
    EXPECT_EQ(std::string("not really a database").size(), info.Size);

    EXPECT_NE(0u, fs.OpenHandle("/databases/content.nwdb"));
}

TEST(ZipFileSystem, ChunkReadReportsOnlyTheBytesItGot) {
    // The old version always appended the full requested size, so a short final read tacked on
    // whatever happened to be in the buffer.
    TempZip zip;
    const std::string contents(5000, 'q');
    ASSERT_TRUE(zip.Add("blob.bin", contents));

    ZipFileSystem fs(zip.Path());
    ASSERT_FALSE(fs.Fail());

    FSEntryHandle handle = fs.OpenHandle("/blob.bin");
    ASSERT_NE(0u, handle);

    std::vector<unsigned char> all, chunk;
    while (fs.ReadHandleChunk(handle, &chunk, 1024)) {
        all.insert(all.end(), chunk.begin(), chunk.end());
        if (chunk.empty())
            break;
    }
    fs.CloseHandle(handle);

    ASSERT_EQ(contents.size(), all.size());
    EXPECT_EQ(0, std::memcmp(contents.data(), all.data(), contents.size()));
}

TEST(ZipFileSystem, ReportsEofWithoutConsumingTheStream) {
    TempZip zip;
    ASSERT_TRUE(zip.Add("small.txt", "abc"));

    ZipFileSystem fs(zip.Path());
    ASSERT_FALSE(fs.Fail());

    FSEntryHandle handle = fs.OpenHandle("/small.txt");
    ASSERT_NE(0u, handle);

    // Asking repeatedly must not eat anything.
    EXPECT_FALSE(fs.IsHandleEOF(handle));
    EXPECT_FALSE(fs.IsHandleEOF(handle));
    EXPECT_FALSE(fs.IsHandleEOF(handle));

    std::vector<unsigned char> chunk;
    ASSERT_TRUE(fs.ReadHandleChunk(handle, &chunk, 3));
    EXPECT_EQ(3u, chunk.size());
    EXPECT_TRUE(fs.IsHandleEOF(handle));

    fs.CloseHandle(handle);
}
