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
// File: BinaryRobloxFileReader.cpp
// Started by: Hattozo
// Started on: 8/18/2025
// Description: Sequential reader over a decompressed binary chunk body.
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryFileReader.h>

#include <cstring>
#include <limits>

using namespace NoobWarrior::Roblox;

BinaryRobloxFileReader::BinaryRobloxFileReader(std::span<const unsigned char> data) : mData(data) {}

size_t BinaryRobloxFileReader::Position() const {
    return mPosition;
}

size_t BinaryRobloxFileReader::Remaining() const {
    return mPosition >= mData.size() ? 0 : mData.size() - mPosition;
}

bool BinaryRobloxFileReader::Failed() const {
    return mFailed;
}

void BinaryRobloxFileReader::Seek(size_t position) {
    if (position > mData.size()) {
        mFailed = true;
        return;
    }
    mPosition = position;
}

bool BinaryRobloxFileReader::ReadBytes(void *output, size_t size) {
    if (mFailed || Remaining() < size) {
        mFailed = true;
        return false;
    }
    std::memcpy(output, mData.data() + mPosition, size);
    mPosition += size;
    return true;
}

uint8_t BinaryRobloxFileReader::ReadByte() {
    uint8_t value = 0;
    ReadBytes(&value, sizeof(value));
    return value;
}

int32_t BinaryRobloxFileReader::ReadInt32() {
    int32_t value = 0;
    ReadBytes(&value, sizeof(value));
    return value;
}

uint32_t BinaryRobloxFileReader::ReadUInt32() {
    uint32_t value = 0;
    ReadBytes(&value, sizeof(value));
    return value;
}

int64_t BinaryRobloxFileReader::ReadInt64() {
    int64_t value = 0;
    ReadBytes(&value, sizeof(value));
    return value;
}

float BinaryRobloxFileReader::ReadFloat() {
    float value = 0;
    ReadBytes(&value, sizeof(value));
    return value;
}

double BinaryRobloxFileReader::ReadDouble() {
    double value = 0;
    ReadBytes(&value, sizeof(value));
    return value;
}

std::vector<unsigned char> BinaryRobloxFileReader::ReadRawString() {
    const uint32_t size = ReadUInt32();
    std::vector<unsigned char> value;
    if (mFailed || size > Remaining()) {
        mFailed = true;
        return value;
    }
    value.assign(mData.begin() + static_cast<std::ptrdiff_t>(mPosition),
                 mData.begin() + static_cast<std::ptrdiff_t>(mPosition + size));
    mPosition += size;
    return value;
}

std::string BinaryRobloxFileReader::ReadString() {
    const std::vector<unsigned char> raw = ReadRawString();
    return {raw.begin(), raw.end()};
}

bool BinaryRobloxFileReader::ReadInterleaved(size_t count, size_t width,
                                       std::vector<unsigned char> &output) {
    if (count > std::numeric_limits<uint32_t>::max() / (width == 0 ? 1 : width)) {
        mFailed = true;
        return false;
    }
    const size_t total = count * width;
    if (mFailed || Remaining() < total) {
        mFailed = true;
        return false;
    }

    output.assign(total, 0);
    for (size_t index = 0; index < count; ++index) {
        for (size_t byteIndex = 0; byteIndex < width; ++byteIndex) {
            // Big-endian within each value, so byte 0 is the most significant.
            output[index * width + (width - byteIndex - 1)] =
                mData[mPosition + byteIndex * count + index];
        }
    }
    mPosition += total;
    return true;
}

int32_t BinaryRobloxFileReader::RotateInt32(uint32_t value) {
    return static_cast<int32_t>((value >> 1) ^ (~(value & 1) + 1));
}

int64_t BinaryRobloxFileReader::RotateInt64(uint64_t value) {
    return static_cast<int64_t>((value >> 1) ^ (~(value & 1) + 1));
}

float BinaryRobloxFileReader::RotateFloat(uint32_t value) {
    const uint32_t bits = (value >> 1) | (value << 31);
    float result = 0;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}

std::vector<int32_t> BinaryRobloxFileReader::ReadInts(size_t count) {
    std::vector<unsigned char> raw;
    std::vector<int32_t> values;
    if (!ReadInterleaved(count, sizeof(uint32_t), raw))
        return values;
    values.reserve(count);
    for (size_t index = 0; index < count; ++index) {
        uint32_t encoded = 0;
        std::memcpy(&encoded, raw.data() + index * sizeof(uint32_t), sizeof(encoded));
        values.push_back(RotateInt32(encoded));
    }
    return values;
}

std::vector<int64_t> BinaryRobloxFileReader::ReadLongs(size_t count) {
    std::vector<unsigned char> raw;
    std::vector<int64_t> values;
    if (!ReadInterleaved(count, sizeof(uint64_t), raw))
        return values;
    values.reserve(count);
    for (size_t index = 0; index < count; ++index) {
        uint64_t encoded = 0;
        std::memcpy(&encoded, raw.data() + index * sizeof(uint64_t), sizeof(encoded));
        values.push_back(RotateInt64(encoded));
    }
    return values;
}

std::vector<float> BinaryRobloxFileReader::ReadFloats(size_t count) {
    std::vector<unsigned char> raw;
    std::vector<float> values;
    if (!ReadInterleaved(count, sizeof(uint32_t), raw))
        return values;
    values.reserve(count);
    for (size_t index = 0; index < count; ++index) {
        uint32_t encoded = 0;
        std::memcpy(&encoded, raw.data() + index * sizeof(uint32_t), sizeof(encoded));
        values.push_back(RotateFloat(encoded));
    }
    return values;
}

std::vector<int32_t> BinaryRobloxFileReader::ReadReferents(size_t count) {
    std::vector<int32_t> values = ReadInts(count);
    int32_t previous = 0;
    for (int32_t &value : values) {
        value += previous;
        previous = value;
    }
    return values;
}
