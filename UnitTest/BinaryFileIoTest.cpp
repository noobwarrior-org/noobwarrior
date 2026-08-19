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
// File: BinaryFileIoTest.cpp
// Started by: Hattozo
// Started on: 8/18/2025
// Description: Ported binary chunk reader/writer primitives, cross-checked on installed places.
// Validates the ported BinaryRobloxFileReader/BinaryRobloxFileWriter primitives, including a byte-exact
// cross-check against real place files decoded by the existing implementation.
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryFileReader.h>
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryFileWriter.h>
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryRobloxFile.h>
#include <NoobWarrior/Roblox/FileFormat/RobloxFile.h>

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <memory>
#include <vector>

using NoobWarrior::Roblox::BinaryRobloxFileReader;
using NoobWarrior::Roblox::BinaryRobloxFileWriter;

TEST(BinaryFileIo, ZigzagAndFloatRotationRoundTrip) {
    const std::vector<int32_t> ints {0, 1, -1, 2, -2, 12345, -12345,
                                     std::numeric_limits<int32_t>::min(),
                                     std::numeric_limits<int32_t>::max()};
    for (int32_t value : ints)
        EXPECT_EQ(value, BinaryRobloxFileReader::RotateInt32(BinaryRobloxFileWriter::RotateInt32(value)));

    const std::vector<int64_t> longs {0, 1, -1, 1234567890123LL, -1234567890123LL,
                                      std::numeric_limits<int64_t>::min(),
                                      std::numeric_limits<int64_t>::max()};
    for (int64_t value : longs)
        EXPECT_EQ(value, BinaryRobloxFileReader::RotateInt64(BinaryRobloxFileWriter::RotateInt64(value)));

    const std::vector<float> floats {0.0f, 1.0f, -1.0f, 0.5f, -123.456f, 3.4028235e38f};
    for (float value : floats)
        EXPECT_EQ(value, BinaryRobloxFileReader::RotateFloat(BinaryRobloxFileWriter::RotateFloat(value)));
}

TEST(BinaryFileIo, ColumnsRoundTripThroughInterleaving) {
    const std::vector<int32_t> ints {5, -7, 0, 99999, -99999};
    const std::vector<int64_t> longs {5, -7, 0, 8589934592LL};
    const std::vector<float> floats {1.5f, -2.25f, 0.0f, 1e-8f};
    const std::vector<int32_t> referents {0, 1, 2, 10, 9, 400, -1};

    BinaryRobloxFileWriter writer;
    writer.WriteInts(ints);
    writer.WriteLongs(longs);
    writer.WriteFloats(floats);
    writer.WriteReferents(referents);
    writer.WriteString("hello world");

    BinaryRobloxFileReader reader(writer.Data());
    EXPECT_EQ(ints, reader.ReadInts(ints.size()));
    EXPECT_EQ(longs, reader.ReadLongs(longs.size()));
    EXPECT_EQ(floats, reader.ReadFloats(floats.size()));
    EXPECT_EQ(referents, reader.ReadReferents(referents.size()));
    EXPECT_EQ("hello world", reader.ReadString());
    EXPECT_FALSE(reader.Failed());
    EXPECT_EQ(0u, reader.Remaining());
}

TEST(BinaryFileIo, ShortReadFailsInsteadOfOverrunning) {
    const std::vector<unsigned char> data {0x04, 0x00, 0x00, 0x00, 'a', 'b'};
    BinaryRobloxFileReader reader(data);
    EXPECT_TRUE(reader.ReadString().empty());
    EXPECT_TRUE(reader.Failed());
}

// Decodes every INST chunk of every installed place with the ported reader and compares against
// the referent table the existing implementation produced, then re-encodes and requires the bytes
// to match the original file exactly.
TEST(BinaryFileIo, MatchesExistingDecoderOnInstalledPlaces) {
    const char *root = std::getenv("NOOBWARRIOR_ENGINES_DIR");
    if (root == nullptr)
        GTEST_SKIP() << "set NOOBWARRIOR_ENGINES_DIR to cross-check against installed places";

    int chunksChecked = 0;
    int placesChecked = 0;
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
        ++placesChecked;

        for (const auto &chunk : document.Chunks) {
            if (!chunk.Is("INST"))
                continue;

            BinaryRobloxFileReader reader(chunk.Data);
            const uint32_t classId = reader.ReadUInt32();
            const std::string className = reader.ReadString();
            const uint8_t format = reader.ReadByte();
            const uint32_t count = reader.ReadUInt32();
            const size_t referentsStart = reader.Position();
            const std::vector<int32_t> referents = reader.ReadReferents(count);
            ASSERT_FALSE(reader.Failed()) << iterator->path().string();

            const auto *expected = document.FindClass(static_cast<int32_t>(classId));
            ASSERT_NE(nullptr, expected) << iterator->path().string();
            EXPECT_EQ(expected->ClassName, className);
            EXPECT_EQ(expected->ObjectIds, referents)
                << "INST " << className << " in " << iterator->path().string();
            EXPECT_EQ(format == 1, expected->IsService);

            BinaryRobloxFileWriter writer;
            writer.WriteReferents(referents);
            const std::vector<unsigned char> original(
                chunk.Data.begin() + static_cast<std::ptrdiff_t>(referentsStart),
                chunk.Data.begin() + static_cast<std::ptrdiff_t>(reader.Position()));
            EXPECT_EQ(original, writer.Data())
                << "re-encoded referents differ for " << className
                << " in " << iterator->path().string();
            ++chunksChecked;
        }
    }

    EXPECT_GT(placesChecked, 0);
    EXPECT_GT(chunksChecked, 0);
    std::cout << "cross-checked " << chunksChecked << " INST chunks across "
              << placesChecked << " places\n";
}
