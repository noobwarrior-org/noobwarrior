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
// File: PRNTTest.cpp
// Started by: Hattozo
// Started on: 8/18/2025
// Description: PRNT chunk, re-encoded byte-for-byte against installed places.
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/Chunks/PRNT.h>

#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryRobloxFile.h>
#include <NoobWarrior/Roblox/FileFormat/RobloxFile.h>

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <vector>

namespace {
// Walks every installed .rbxl, handing each chunk of the requested type to the callback.
template<typename Visitor>
int ForEachChunk(const char *type, Visitor &&visit) {
    const char *root = std::getenv("NOOBWARRIOR_ENGINES_DIR");
    if (root == nullptr)
        return -1;

    int visited = 0;
    std::error_code code;
    for (auto iterator = std::filesystem::recursive_directory_iterator(root, code);
         !code && iterator != std::filesystem::recursive_directory_iterator();
         iterator.increment(code)) {
        if (iterator->path().extension() != ".rbxl")
            continue;

        std::ifstream stream(iterator->path(), std::ios::binary);
        const std::vector<unsigned char> bytes(
            (std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        if (bytes.empty())
            continue;

        NoobWarrior::Roblox::BinaryFormat::BinaryRobloxFile document;
        std::string loadError;
        if (!document.Load(bytes, &loadError))
            continue;

        for (const auto &chunk : document.Chunks) {
            if (!chunk.Is(type))
                continue;
            visit(document, chunk, iterator->path().string());
            ++visited;
        }
    }
    return visited;
}
} // namespace

using NoobWarrior::Roblox::PRNT;
using NoobWarrior::Roblox::BinaryRobloxFileReader;
using NoobWarrior::Roblox::BinaryRobloxFileWriter;

TEST(PRNTChunk, RoundTripsSyntheticChunk) {
    PRNT chunk;
    chunk.ChildIds = {1, 2, 3, 10, 9};
    chunk.ParentIds = {-1, 1, 1, 2, -1};

    BinaryRobloxFileWriter writer;
    chunk.Save(writer);

    PRNT decoded;
    BinaryRobloxFileReader reader(writer.Data());
    ASSERT_TRUE(decoded.Load(reader));
    EXPECT_EQ(0u, reader.Remaining());
    EXPECT_EQ(chunk.ChildIds, decoded.ChildIds);
    EXPECT_EQ(chunk.ParentIds, decoded.ParentIds);
}

TEST(PRNTChunk, RejectsUnexpectedFormatByte) {
    BinaryRobloxFileWriter writer;
    writer.WriteByte(1);
    writer.WriteInt32(0);

    PRNT decoded;
    BinaryRobloxFileReader reader(writer.Data());
    EXPECT_FALSE(decoded.Load(reader));
}

TEST(PRNTChunk, ReencodesInstalledPlacesByteForByte) {
    int mismatches = 0;
    const int visited = ForEachChunk("PRNT", [&](const auto &, const auto &chunk,
                                                const std::string &path) {
        PRNT decoded;
        BinaryRobloxFileReader reader(chunk.Data);
        if (!decoded.Load(reader) || reader.Remaining() != 0) {
            ADD_FAILURE() << "failed to load the PRNT chunk in " << path;
            return;
        }

        BinaryRobloxFileWriter writer;
        decoded.Save(writer);
        if (chunk.Data != writer.Data()) {
            ++mismatches;
            ADD_FAILURE() << "PRNT re-encoded differently in " << path;
        }
    });

    if (visited < 0)
        GTEST_SKIP() << "set NOOBWARRIOR_ENGINES_DIR to cross-check against installed places";
    std::cout << "re-encoded " << visited << " PRNT chunks (" << mismatches << " mismatches)\n";
    EXPECT_GT(visited, 0);
}
