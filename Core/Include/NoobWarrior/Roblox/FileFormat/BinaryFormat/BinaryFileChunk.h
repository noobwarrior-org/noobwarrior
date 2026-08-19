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
// File: BinaryFileChunk.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: One compressed chunk of a binary Roblox file.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/Interfaces/IBinaryFileChunk.h>
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryFileReader.h>
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryFileWriter.h>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace NoobWarrior::Roblox {
// A chunk header is a 4-byte type tag, the compressed and uncompressed sizes, and a reserved
// word. A compressed size of zero means the payload is stored verbatim.
//
// Roblox writes LZ4 exclusively, and so does this class; zstd and zlib payloads are accepted on
// read because some tooling produces them. Note that recompressing does not reproduce Roblox's
// exact bytes, since encoder settings differ, so round-trip equality is defined on the
// decompressed payload rather than the stored one.
class BinaryRobloxFileChunk : public IBinaryFileChunk {
public:
    std::array<unsigned char, 4> ChunkType {};
    int32_t Reserved {};
    int32_t CompressedSize {};
    int32_t Size {};
    std::vector<unsigned char> CompressedData;
    std::vector<unsigned char> Data;

    bool HasCompressedData() const;
    std::string GetChunkType() const;
    bool Is(std::string_view type) const;

    bool Load(BinaryRobloxFileReader &reader, std::string *error = nullptr) override;
    // Passing compress=false, or a payload LZ4 fails to shrink, stores the payload verbatim.
    bool Save(BinaryRobloxFileWriter &writer, bool compress = true,
              std::string *error = nullptr) const override;
    // BinaryFileChunk.cs:32-35 formats the same summary as ToString(). The reference
    // appends its Handler's description too; this port keeps no handler on the chunk.
    void WriteInfo(std::string &builder) const override;

    static bool Decompress(std::span<const unsigned char> stored, size_t uncompressedSize,
                           std::vector<unsigned char> &output, std::string *error = nullptr);
    static bool CompressLz4(std::span<const unsigned char> input,
                            std::vector<unsigned char> &output, std::string *error = nullptr);
};
}
