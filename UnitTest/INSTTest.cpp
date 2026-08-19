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
// File: INSTTest.cpp
// Started by: Hattozo
// Started on: 8/18/2025
// Description: INST chunk, re-encoded byte-for-byte against installed places.
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/Chunks/INST.h>

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

using NoobWarrior::Roblox::INST;
using NoobWarrior::Roblox::BinaryRobloxFileReader;
using NoobWarrior::Roblox::BinaryRobloxFileWriter;

TEST(INSTChunk, RoundTripsSyntheticChunk) {
    INST chunk;
    chunk.ClassIndex = 3;
    chunk.ClassName = "Part";
    chunk.IsService = false;
    chunk.NumObjects = 4;
    chunk.ObjectIds = {0, 5, 4, 100};

    BinaryRobloxFileWriter writer;
    chunk.Save(writer);

    INST decoded;
    BinaryRobloxFileReader reader(writer.Data());
    ASSERT_TRUE(decoded.Load(reader));
    EXPECT_EQ(0u, reader.Remaining());
    EXPECT_EQ(chunk.ClassIndex, decoded.ClassIndex);
    EXPECT_EQ(chunk.ClassName, decoded.ClassName);
    EXPECT_EQ(chunk.IsService, decoded.IsService);
    EXPECT_EQ(chunk.ObjectIds, decoded.ObjectIds);
    EXPECT_TRUE(decoded.RootedServices.empty());
}

TEST(INSTChunk, RoundTripsServiceChunkWithRootedFlags) {
    INST chunk;
    chunk.ClassIndex = 0;
    chunk.ClassName = "Workspace";
    chunk.IsService = true;
    chunk.NumObjects = 3;
    chunk.ObjectIds = {1, 2, 3};
    chunk.RootedServices = {true, false, true};

    BinaryRobloxFileWriter writer;
    chunk.Save(writer);

    INST decoded;
    BinaryRobloxFileReader reader(writer.Data());
    ASSERT_TRUE(decoded.Load(reader));
    EXPECT_EQ(0u, reader.Remaining());
    EXPECT_TRUE(decoded.IsService);
    EXPECT_EQ(chunk.RootedServices, decoded.RootedServices);
}

TEST(INSTChunk, ReencodesInstalledPlacesByteForByte) {
    int mismatches = 0;
    int failures = 0;
    const int visited = ForEachChunk("INST", [&](const auto &binary, const auto &chunk,
                                                 const std::string &path) {
        INST decoded;
        BinaryRobloxFileReader reader(chunk.Data);
        if (!decoded.Load(reader) || reader.Remaining() != 0) {
            ++failures;
            ADD_FAILURE() << "failed to load an INST chunk in " << path;
            return;
        }

        const auto *expected = binary.FindClass(decoded.ClassIndex);
        if (expected != nullptr) {
            EXPECT_EQ(expected->ClassName, decoded.ClassName) << path;
            EXPECT_EQ(expected->ObjectIds, decoded.ObjectIds) << path;
        }

        BinaryRobloxFileWriter writer;
        decoded.Save(writer);
        if (chunk.Data != writer.Data()) {
            ++mismatches;
            ADD_FAILURE() << "INST " << decoded.ClassName << " re-encoded differently in " << path;
        }
    });

    if (visited < 0)
        GTEST_SKIP() << "set NOOBWARRIOR_ENGINES_DIR to cross-check against installed places";
    std::cout << "re-encoded " << visited << " INST chunks (" << mismatches
              << " mismatches, " << failures << " load failures)\n";
    EXPECT_GT(visited, 0);
}
