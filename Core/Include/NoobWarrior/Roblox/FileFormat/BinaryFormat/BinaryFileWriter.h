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
// File: BinaryFileWriter.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Sequential writer producing a decompressed binary chunk body.
#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace NoobWarrior::Roblox {
// Sequential writer producing one decompressed chunk body. Mirrors BinaryRobloxFileWriter's
// primitive half: the same transposed column layout the reader consumes.
class BinaryRobloxFileWriter {
public:
    const std::vector<unsigned char> &Data() const;
    std::vector<unsigned char> Release();
    size_t Size() const;

    void WriteBytes(const void *data, size_t size);
    void WriteByte(uint8_t value);
    void WriteInt32(int32_t value);
    void WriteUInt32(uint32_t value);
    void WriteInt64(int64_t value);
    void WriteFloat(float value);
    void WriteDouble(double value);
    void WriteString(std::string_view value);
    void WriteRawString(std::span<const unsigned char> value);

    void WriteInterleaved(std::span<const unsigned char> values, size_t count, size_t width);

    void WriteInts(std::span<const int32_t> values);
    void WriteLongs(std::span<const int64_t> values);
    void WriteFloats(std::span<const float> values);
    void WriteReferents(std::span<const int32_t> values);

    static uint32_t RotateInt32(int32_t value);
    static uint64_t RotateInt64(int64_t value);
    static uint32_t RotateFloat(float value);

private:
    std::vector<unsigned char> mData;
};
}
