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
// File: BinaryFileChunkTest.cpp
// Started by: Hattozo
// Started on: 8/18/2025
// Description: Chunk compression layer, exercised over every chunk of every installed place.
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryFileChunk.h>
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryRobloxFile.h>
#include <NoobWarrior/Roblox/FileFormat/RobloxFile.h>

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <numeric>
#include <vector>

using namespace NoobWarrior::Roblox;

TEST(BinaryFileChunkLayer, CompressesAndDecompressesRoundTrip) {
    std::vector<unsigned char> payload(4096);
    std::iota(payload.begin(), payload.end(), 0);

    std::vector<unsigned char> compressed;
    ASSERT_TRUE(BinaryRobloxFileChunk::CompressLz4(payload, compressed));
    EXPECT_LT(compressed.size(), payload.size()) << "repetitive payload should shrink";

    std::vector<unsigned char> restored;
    ASSERT_TRUE(BinaryRobloxFileChunk::Decompress(compressed, payload.size(), restored));
    EXPECT_EQ(payload, restored);
}

TEST(BinaryFileChunkLayer, StoresIncompressiblePayloadVerbatim) {
    // A tiny payload cannot shrink, so the writer must fall back to storing it.
    BinaryRobloxFileChunk chunk;
    chunk.ChunkType = {'M', 'E', 'T', 'A'};
    chunk.Data = {0x01};

    BinaryRobloxFileWriter writer;
    ASSERT_TRUE(chunk.Save(writer));

    BinaryRobloxFileChunk decoded;
    BinaryRobloxFileReader reader(writer.Data());
    ASSERT_TRUE(decoded.Load(reader));
    EXPECT_EQ(0, decoded.CompressedSize);
    EXPECT_FALSE(decoded.HasCompressedData());
    EXPECT_EQ(chunk.Data, decoded.Data);
    EXPECT_EQ("META", decoded.GetChunkType());
    EXPECT_TRUE(decoded.Is("META"));
}

TEST(BinaryFileChunkLayer, RejectsChunkExtendingPastEndOfFile) {
    BinaryRobloxFileWriter writer;
    writer.WriteBytes("INST", 4);
    writer.WriteInt32(0);
    writer.WriteInt32(9999);
    writer.WriteInt32(0);

    BinaryRobloxFileChunk chunk;
    BinaryRobloxFileReader reader(writer.Data());
    std::string error;
    EXPECT_FALSE(chunk.Load(reader, &error));
    EXPECT_NE(std::string::npos, error.find("past the end"));
}

// Every chunk of every installed place is re-saved through the LZ4 encoder and read back. The
// stored bytes deliberately are not compared: Roblox's encoder settings differ, so the invariant
// is that the decompressed payload survives a compress/decompress cycle unchanged.
TEST(BinaryFileChunkLayer, PayloadSurvivesRecompressionOnInstalledPlaces) {
    const char *root = std::getenv("NOOBWARRIOR_ENGINES_DIR");
    if (root == nullptr)
        GTEST_SKIP() << "set NOOBWARRIOR_ENGINES_DIR to cross-check against installed places";

    int chunks = 0;
    int places = 0;
    uint64_t originalBytes = 0;
    uint64_t compressedBytes = 0;
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
        ++places;

        for (const auto &existing : document.Chunks) {
            BinaryRobloxFileChunk chunk;
            std::copy(existing.ChunkType.begin(), existing.ChunkType.end(), chunk.ChunkType.begin());
            chunk.Reserved = static_cast<int32_t>(existing.Reserved);
            chunk.Data = existing.Data;

            BinaryRobloxFileWriter writer;
            std::string error;
            ASSERT_TRUE(chunk.Save(writer, true, &error)) << error;

            BinaryRobloxFileChunk decoded;
            BinaryRobloxFileReader reader(writer.Data());
            ASSERT_TRUE(decoded.Load(reader, &error))
                << error << " in " << iterator->path().string();
            ASSERT_EQ(0u, reader.Remaining());
            ASSERT_EQ(chunk.Data, decoded.Data)
                << chunk.GetChunkType() << " payload changed in " << iterator->path().string();

            originalBytes += chunk.Data.size();
            compressedBytes += writer.Data().size();
            ++chunks;
        }
    }

    std::cout << "recompressed " << chunks << " chunks across " << places << " places; "
              << originalBytes << " -> " << compressedBytes << " bytes ("
              << (originalBytes == 0 ? 0 : compressedBytes * 100 / originalBytes) << "%)\n";
    EXPECT_GT(chunks, 0);
}
