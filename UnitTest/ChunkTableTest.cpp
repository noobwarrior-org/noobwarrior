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
// File: ChunkTableTest.cpp
// Started by: Hattozo
// Started on: 8/18/2025
// Description: SSTR, META and SIGN chunks, re-encoded byte-for-byte against installed places.
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/Chunks/META.h>
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/Chunks/SIGN.h>
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/Chunks/SSTR.h>
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

using namespace NoobWarrior::Roblox;

namespace {
template<typename Chunk>
int ReencodeAll(const char *type, int &mismatches) {
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
            Chunk decoded;
            BinaryRobloxFileReader reader(chunk.Data);
            if (!decoded.Load(reader) || reader.Remaining() != 0) {
                ADD_FAILURE() << "failed to load a " << type << " chunk in "
                              << iterator->path().string();
                continue;
            }
            BinaryRobloxFileWriter writer;
            decoded.Save(writer);
            if (chunk.Data != writer.Data()) {
                ++mismatches;
                ADD_FAILURE() << type << " re-encoded differently in "
                              << iterator->path().string();
            }
            ++visited;
        }
    }
    return visited;
}
} // namespace

TEST(SSTRChunk, RoundTripsSyntheticChunk) {
    SSTR chunk;
    SSTR::Entry first;
    first.Hash.fill(0x11);
    first.Value = {'a', 'b', 'c'};
    SSTR::Entry second;
    second.Hash.fill(0x22);
    second.Value = {};

    EXPECT_EQ(0u, chunk.Add(first));
    EXPECT_EQ(1u, chunk.Add(second));
    EXPECT_EQ(0u, chunk.Add(first)) << "adding a known hash must reuse its id";
    ASSERT_TRUE(chunk.Find(first.Hash).has_value());

    BinaryRobloxFileWriter writer;
    chunk.Save(writer);

    SSTR decoded;
    BinaryRobloxFileReader reader(writer.Data());
    ASSERT_TRUE(decoded.Load(reader));
    EXPECT_EQ(0u, reader.Remaining());
    EXPECT_EQ(chunk.Strings, decoded.Strings);
}

TEST(METAChunk, RoundTripsAndPreservesOrder) {
    META chunk;
    chunk.Data = {{"ExplicitAutoJoints", "true"}, {"Alpha", "1"}};

    BinaryRobloxFileWriter writer;
    chunk.Save(writer);

    META decoded;
    BinaryRobloxFileReader reader(writer.Data());
    ASSERT_TRUE(decoded.Load(reader));
    EXPECT_EQ(0u, reader.Remaining());
    EXPECT_EQ(chunk.Data, decoded.Data);
    ASSERT_NE(nullptr, decoded.Find("ExplicitAutoJoints"));
    EXPECT_EQ("true", *decoded.Find("ExplicitAutoJoints"));
    EXPECT_EQ(nullptr, decoded.Find("Missing"));
}

TEST(SIGNChunk, RoundTripsSyntheticChunk) {
    SIGN chunk;
    chunk.Signatures.push_back({RbxSignatureType::Ed25519, 42, {1, 2, 3, 4}});
    chunk.Signatures.push_back({RbxSignatureType::Ed25519, -1, {}});

    BinaryRobloxFileWriter writer;
    chunk.Save(writer);

    SIGN decoded;
    BinaryRobloxFileReader reader(writer.Data());
    ASSERT_TRUE(decoded.Load(reader));
    EXPECT_EQ(0u, reader.Remaining());
    EXPECT_EQ(chunk.Signatures, decoded.Signatures);
}

TEST(ChunkTables, ReencodeInstalledPlacesByteForByte) {
    int mismatches = 0;
    const int shared = ReencodeAll<SSTR>("SSTR", mismatches);
    if (shared < 0)
        GTEST_SKIP() << "set NOOBWARRIOR_ENGINES_DIR to cross-check against installed places";
    const int meta = ReencodeAll<META>("META", mismatches);
    const int sign = ReencodeAll<SIGN>("SIGN", mismatches);

    std::cout << "re-encoded " << shared << " SSTR, " << meta << " META, " << sign
              << " SIGN chunks (" << mismatches << " mismatches)\n";
    EXPECT_EQ(0, mismatches);
}
