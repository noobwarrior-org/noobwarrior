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
// File: BinaryFileReader.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Sequential reader over a decompressed binary chunk body.
#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace NoobWarrior::Roblox {
// Sequential reader over one decompressed chunk body. Mirrors BinaryRobloxFileReader: every
// accessor advances the cursor, and a short read leaves Failed() set rather than throwing.
class BinaryRobloxFileReader {
public:
    explicit BinaryRobloxFileReader(std::span<const unsigned char> data);

    size_t Position() const;
    size_t Remaining() const;
    bool Failed() const;
    void Seek(size_t position);

    bool ReadBytes(void *output, size_t size);
    uint8_t ReadByte();
    int32_t ReadInt32();
    uint32_t ReadUInt32();
    int64_t ReadInt64();
    float ReadFloat();
    double ReadDouble();
    std::string ReadString();
    std::vector<unsigned char> ReadRawString();

    // Roblox stores multi-byte columns transposed: every value's Nth byte is grouped together.
    bool ReadInterleaved(size_t count, size_t width, std::vector<unsigned char> &output);

    std::vector<int32_t> ReadInts(size_t count);
    std::vector<int64_t> ReadLongs(size_t count);
    std::vector<float> ReadFloats(size_t count);
    // Referent columns are additionally delta-encoded against the previous value.
    std::vector<int32_t> ReadReferents(size_t count);

    static int32_t RotateInt32(uint32_t value);
    static int64_t RotateInt64(uint64_t value);
    static float RotateFloat(uint32_t value);

private:
    std::span<const unsigned char> mData;
    size_t mPosition {0};
    bool mFailed {false};
};
}
