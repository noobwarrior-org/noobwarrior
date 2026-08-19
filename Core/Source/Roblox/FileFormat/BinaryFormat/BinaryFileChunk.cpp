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
// File: BinaryFileChunk.cpp
// Started by: Hattozo
// Started on: 8/18/2025
// Description: One compressed chunk of a binary Roblox file.
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryFileChunk.h>

#include <lz4.h>
#include <zlib.h>
#include <zstd.h>

#include <algorithm>
#include <limits>

using namespace NoobWarrior::Roblox;

namespace {
constexpr size_t kMaximumChunkSize = 512 * 1024 * 1024;

void SetError(std::string *error, std::string message) {
    if (error != nullptr)
        *error = std::move(message);
}

bool IsZstd(std::span<const unsigned char> data) {
    return data.size() >= 4 && data[0] == 0x28 && data[1] == 0xb5 &&
           data[2] == 0x2f && data[3] == 0xfd;
}

bool IsZlib(std::span<const unsigned char> data) {
    return data.size() >= 2 && (data[0] == 0x78 || data[0] == 0x58);
}
} // namespace

bool BinaryRobloxFileChunk::HasCompressedData() const {
    return CompressedSize > 0;
}

std::string BinaryRobloxFileChunk::GetChunkType() const {
    const auto end = std::find(ChunkType.begin(), ChunkType.end(), 0);
    return std::string(ChunkType.begin(), end);
}

bool BinaryRobloxFileChunk::Is(std::string_view type) const {
    if (type.size() > ChunkType.size())
        return false;
    for (size_t index = 0; index < ChunkType.size(); ++index) {
        const unsigned char expected = index < type.size()
            ? static_cast<unsigned char>(type[index]) : 0;
        if (ChunkType[index] != expected)
            return false;
    }
    return true;
}

// BinaryFileChunk.cs:32-35 renders the type tag with its NUL padding turned into spaces, so a
// short tag still occupies four columns and chunk lines stay aligned with one another.
void BinaryRobloxFileChunk::WriteInfo(std::string &builder) const {
    builder += "'";
    for (unsigned char byte : ChunkType)
        builder += byte == 0 ? ' ' : static_cast<char>(byte);
    builder += "' Chunk (";
    builder += std::to_string(Size);
    builder += " bytes)";
}

bool BinaryRobloxFileChunk::Decompress(std::span<const unsigned char> stored,
                                       size_t uncompressedSize,
                                       std::vector<unsigned char> &output,
                                       std::string *error) {
    output.assign(uncompressedSize, 0);
    if (uncompressedSize == 0)
        return true;

    if (IsZstd(stored)) {
        const size_t result = ZSTD_decompress(output.data(), output.size(),
                                              stored.data(), stored.size());
        if (ZSTD_isError(result) || result != uncompressedSize) {
            SetError(error, "could not decompress a ZSTD chunk");
            return false;
        }
        return true;
    }

    if (IsZlib(stored)) {
        uLongf produced = static_cast<uLongf>(uncompressedSize);
        const int result = uncompress(output.data(), &produced, stored.data(),
                                      static_cast<uLong>(stored.size()));
        if (result == Z_OK && produced == uncompressedSize)
            return true;
        // Fall through: a payload can begin with 0x78 by coincidence and still be LZ4.
    }

    const int produced = LZ4_decompress_safe(
        reinterpret_cast<const char *>(stored.data()),
        reinterpret_cast<char *>(output.data()),
        static_cast<int>(stored.size()), static_cast<int>(output.size()));
    if (produced < 0 || static_cast<size_t>(produced) != uncompressedSize) {
        SetError(error, "could not decompress an LZ4 chunk");
        return false;
    }
    return true;
}

bool BinaryRobloxFileChunk::CompressLz4(std::span<const unsigned char> input,
                                        std::vector<unsigned char> &output,
                                        std::string *error) {
    if (input.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
        SetError(error, "chunk is too large to compress");
        return false;
    }

    const int bound = LZ4_compressBound(static_cast<int>(input.size()));
    if (bound <= 0) {
        SetError(error, "chunk is too large to compress");
        return false;
    }
    output.assign(static_cast<size_t>(bound), 0);
    const int produced = LZ4_compress_default(
        reinterpret_cast<const char *>(input.data()),
        reinterpret_cast<char *>(output.data()),
        static_cast<int>(input.size()), bound);
    if (produced <= 0) {
        SetError(error, "could not compress a chunk with LZ4");
        return false;
    }
    output.resize(static_cast<size_t>(produced));
    return true;
}

bool BinaryRobloxFileChunk::Load(BinaryRobloxFileReader &reader, std::string *error) {
    if (!reader.ReadBytes(ChunkType.data(), ChunkType.size())) {
        SetError(error, "truncated chunk header");
        return false;
    }
    CompressedSize = reader.ReadInt32();
    Size = reader.ReadInt32();
    Reserved = reader.ReadInt32();
    if (reader.Failed() || CompressedSize < 0 || Size < 0 ||
        static_cast<size_t>(Size) > kMaximumChunkSize) {
        SetError(error, "invalid chunk header");
        return false;
    }

    const size_t stored = CompressedSize == 0 ? static_cast<size_t>(Size)
                                              : static_cast<size_t>(CompressedSize);
    if (stored > reader.Remaining()) {
        SetError(error, "chunk extends past the end of the file");
        return false;
    }

    if (CompressedSize == 0) {
        CompressedData.clear();
        Data.assign(stored, 0);
        return reader.ReadBytes(Data.data(), stored);
    }

    CompressedData.assign(stored, 0);
    if (!reader.ReadBytes(CompressedData.data(), stored)) {
        SetError(error, "truncated chunk payload");
        return false;
    }
    return Decompress(CompressedData, static_cast<size_t>(Size), Data, error);
}

bool BinaryRobloxFileChunk::Save(BinaryRobloxFileWriter &writer, bool compress,
                                 std::string *error) const {
    std::vector<unsigned char> payload;
    bool stored = true;
    if (compress) {
        if (!CompressLz4(Data, payload, error))
            return false;
        stored = payload.size() >= Data.size();
    }
    if (stored)
        payload.assign(Data.begin(), Data.end());

    writer.WriteBytes(ChunkType.data(), ChunkType.size());
    writer.WriteInt32(stored ? 0 : static_cast<int32_t>(payload.size()));
    writer.WriteInt32(static_cast<int32_t>(Data.size()));
    writer.WriteInt32(Reserved);
    writer.WriteBytes(payload.data(), payload.size());
    return true;
}
