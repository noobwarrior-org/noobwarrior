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
// File: BinaryRobloxFileWriter.cpp
// Started by: Hattozo
// Started on: 8/18/2025
// Description: Sequential writer producing a decompressed binary chunk body.
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryFileWriter.h>

#include <cstring>

using namespace NoobWarrior::Roblox;

const std::vector<unsigned char> &BinaryRobloxFileWriter::Data() const {
    return mData;
}

std::vector<unsigned char> BinaryRobloxFileWriter::Release() {
    return std::move(mData);
}

size_t BinaryRobloxFileWriter::Size() const {
    return mData.size();
}

void BinaryRobloxFileWriter::WriteBytes(const void *data, size_t size) {
    const auto *bytes = static_cast<const unsigned char *>(data);
    mData.insert(mData.end(), bytes, bytes + size);
}

void BinaryRobloxFileWriter::WriteByte(uint8_t value) {
    mData.push_back(value);
}

void BinaryRobloxFileWriter::WriteInt32(int32_t value) {
    WriteBytes(&value, sizeof(value));
}

void BinaryRobloxFileWriter::WriteUInt32(uint32_t value) {
    WriteBytes(&value, sizeof(value));
}

void BinaryRobloxFileWriter::WriteInt64(int64_t value) {
    WriteBytes(&value, sizeof(value));
}

void BinaryRobloxFileWriter::WriteFloat(float value) {
    WriteBytes(&value, sizeof(value));
}

void BinaryRobloxFileWriter::WriteDouble(double value) {
    WriteBytes(&value, sizeof(value));
}

void BinaryRobloxFileWriter::WriteRawString(std::span<const unsigned char> value) {
    WriteUInt32(static_cast<uint32_t>(value.size()));
    mData.insert(mData.end(), value.begin(), value.end());
}

void BinaryRobloxFileWriter::WriteString(std::string_view value) {
    WriteUInt32(static_cast<uint32_t>(value.size()));
    mData.insert(mData.end(), value.begin(), value.end());
}

void BinaryRobloxFileWriter::WriteInterleaved(std::span<const unsigned char> values, size_t count,
                                        size_t width) {
    const size_t start = mData.size();
    mData.resize(start + count * width);
    for (size_t index = 0; index < count; ++index) {
        for (size_t byteIndex = 0; byteIndex < width; ++byteIndex) {
            mData[start + byteIndex * count + index] =
                values[index * width + (width - byteIndex - 1)];
        }
    }
}

uint32_t BinaryRobloxFileWriter::RotateInt32(int32_t value) {
    return (static_cast<uint32_t>(value) << 1) ^ static_cast<uint32_t>(value >> 31);
}

uint64_t BinaryRobloxFileWriter::RotateInt64(int64_t value) {
    return (static_cast<uint64_t>(value) << 1) ^ static_cast<uint64_t>(value >> 63);
}

uint32_t BinaryRobloxFileWriter::RotateFloat(float value) {
    uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return (bits << 1) | (bits >> 31);
}

void BinaryRobloxFileWriter::WriteInts(std::span<const int32_t> values) {
    std::vector<unsigned char> raw(values.size() * sizeof(uint32_t));
    for (size_t index = 0; index < values.size(); ++index) {
        const uint32_t encoded = RotateInt32(values[index]);
        std::memcpy(raw.data() + index * sizeof(uint32_t), &encoded, sizeof(encoded));
    }
    WriteInterleaved(raw, values.size(), sizeof(uint32_t));
}

void BinaryRobloxFileWriter::WriteLongs(std::span<const int64_t> values) {
    std::vector<unsigned char> raw(values.size() * sizeof(uint64_t));
    for (size_t index = 0; index < values.size(); ++index) {
        const uint64_t encoded = RotateInt64(values[index]);
        std::memcpy(raw.data() + index * sizeof(uint64_t), &encoded, sizeof(encoded));
    }
    WriteInterleaved(raw, values.size(), sizeof(uint64_t));
}

void BinaryRobloxFileWriter::WriteFloats(std::span<const float> values) {
    std::vector<unsigned char> raw(values.size() * sizeof(uint32_t));
    for (size_t index = 0; index < values.size(); ++index) {
        const uint32_t encoded = RotateFloat(values[index]);
        std::memcpy(raw.data() + index * sizeof(uint32_t), &encoded, sizeof(encoded));
    }
    WriteInterleaved(raw, values.size(), sizeof(uint32_t));
}

void BinaryRobloxFileWriter::WriteReferents(std::span<const int32_t> values) {
    std::vector<int32_t> deltas;
    deltas.reserve(values.size());
    int32_t previous = 0;
    for (int32_t value : values) {
        deltas.push_back(value - previous);
        previous = value;
    }
    WriteInts(deltas);
}
