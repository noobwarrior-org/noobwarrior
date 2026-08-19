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
// File: IAttributeToken.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: This file is derived from Roblox-File-Format (https://github.com/MaximumADHD/Roblox-File-Format/blob/main/Interfaces/IAttributeToken.cs)
#pragma once
#include <NoobWarrior/Roblox/FileFormat/DataTypes/DataTypes.h>

#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace NoobWarrior::Roblox {
/**
 * @brief The type tag an attribute carries in the AttributesSerialize blob.
 *
 * Reference Tree/Attributes.cs:14-47. These are NOT PropertyType values -- attributes have their
 * own numbering, and the gaps are the types the reference leaves commented out because Roblox
 * writes them but the reference cannot decode them.
 */
enum class AttributeType : uint8_t {
    // A value that never appears on the wire; it marks an attribute this port has not decoded.
    None = 0,
 // Null = 1,
    String = 2,
    Bool = 3,
    Int = 4,
    Float = 5,
    Double = 6,
 // Array = 7,
 // Dictionary = 8,
    UDim = 9,
    UDim2 = 10,
 // Ray = 11,
 // Faces = 12,
 // Axes = 13,
    BrickColor = 14,
    Color3 = 15,
    Vector2 = 16,
    Vector3 = 17,
 // Vector2int16 = 18,
 // Vector3int16 = 19,
    CFrame = 20,
    Enum = 21,
    NumberSequence = 23,
 // NumberSequenceKeypoint = 24,
    ColorSequence = 25,
 // ColorSequenceKeypoint = 26,
    NumberRange = 27,
    Rect = 28,
 // PhysicalProperties = 29,
 // Region3 = 31,
 // Region3int16 = 32,
    FontFace = 33,
};

/**
 * @brief The value of an Enum attribute.
 *
 * Tokens/Enum.cs:64-80 resolves the serialized enum name against the RobloxFiles.Enums namespace
 * by reflection and drops the attribute when that lookup fails. C++ has no such lookup, and
 * dropping an attribute the file actually carries would lose user data, so the name is kept
 * verbatim alongside the numeric value -- which is also what makes the blob round-trip.
 */
struct RbxAttributeEnum {
    std::string Name;
    uint32_t Value {};

    RbxAttributeEnum() = default;
    RbxAttributeEnum(std::string name, uint32_t value) : Name(std::move(name)), Value(value) {}

    friend bool operator==(const RbxAttributeEnum &, const RbxAttributeEnum &) = default;
};

/**
 * @brief The little-endian byte reader an attribute token decodes through.
 *
 * Reference Attributes.cs:144-154 hangs these off RbxAttribute itself; they live here because the
 * tokens below are what use them, and a token header that had to include Attributes.h would close
 * an include cycle.
 *
 * Every read is bounds-checked and a failed read latches: a token can decode straight through and
 * let the caller test Failed() once, exactly as the reference lets a BinaryReader throw once.
 */
class AttributeReader {
public:
    AttributeReader(const unsigned char *data, size_t size) : mData(data), mSize(size) {}

    explicit AttributeReader(std::string_view data) :
        mData(reinterpret_cast<const unsigned char *>(data.data())), mSize(data.size()) {}

    bool Failed() const { return mFailed; }
    size_t Position() const { return mPosition; }
    size_t Remaining() const { return mFailed ? 0 : mSize - mPosition; }

    /// Latches failure without consuming anything, for a length field a token knows is nonsense.
    void Fail() { mFailed = true; }

    uint8_t ReadByte() {
        if (Remaining() < 1)
            return FailWith<uint8_t>();
        return mData[mPosition++];
    }

    bool ReadBool() { return ReadByte() != 0; }

    uint16_t ReadUShort() {
        if (Remaining() < 2)
            return FailWith<uint16_t>();
        const uint16_t value = static_cast<uint16_t>(
            static_cast<uint16_t>(mData[mPosition]) |
            static_cast<uint16_t>(static_cast<uint16_t>(mData[mPosition + 1]) << 8));
        mPosition += 2;
        return value;
    }

    int16_t ReadShort() { return static_cast<int16_t>(ReadUShort()); }

    uint32_t ReadUInt() {
        if (Remaining() < 4)
            return FailWith<uint32_t>();
        const uint32_t value =
            static_cast<uint32_t>(mData[mPosition]) |
            (static_cast<uint32_t>(mData[mPosition + 1]) << 8) |
            (static_cast<uint32_t>(mData[mPosition + 2]) << 16) |
            (static_cast<uint32_t>(mData[mPosition + 3]) << 24);
        mPosition += 4;
        return value;
    }

    int32_t ReadInt() { return static_cast<int32_t>(ReadUInt()); }

    uint64_t ReadULong() {
        if (Remaining() < 8)
            return FailWith<uint64_t>();
        uint64_t value = 0;
        for (size_t index = 0; index < 8; ++index)
            value |= static_cast<uint64_t>(mData[mPosition + index]) << (index * 8);
        mPosition += 8;
        return value;
    }

    float ReadFloat() { return std::bit_cast<float>(ReadUInt()); }
    double ReadDouble() { return std::bit_cast<double>(ReadULong()); }

    /**
     * @brief An int32-length-prefixed UTF-8 string.
     *
     * Attributes.cs:154 reads through Formatting.ReadString(true), which is the int-length form
     * rather than the 7-bit-encoded length a bare BinaryReader.ReadString would use.
     */
    std::string ReadString() {
        const int32_t length = ReadInt();
        if (mFailed || length < 0 || static_cast<uint64_t>(length) > Remaining())
            return FailWith<std::string>();
        std::string value(reinterpret_cast<const char *>(mData + mPosition),
                          static_cast<size_t>(length));
        mPosition += static_cast<size_t>(length);
        return value;
    }

private:
    template <typename T>
    T FailWith() {
        mFailed = true;
        return T {};
    }

    const unsigned char *mData;
    size_t mSize;
    size_t mPosition {0};
    bool mFailed {false};
};

/**
 * @brief The little-endian byte writer an attribute token encodes through.
 *
 * Reference Attributes.cs:156-168.
 */
class AttributeWriter {
public:
    explicit AttributeWriter(std::string &out) : mOut(out) {}

    void WriteByte(uint8_t value) { mOut.push_back(static_cast<char>(value)); }
    void WriteBool(bool value) { WriteByte(value ? uint8_t {1} : uint8_t {0}); }

    void WriteUShort(uint16_t value) {
        WriteByte(static_cast<uint8_t>(value & 0xFF));
        WriteByte(static_cast<uint8_t>((value >> 8) & 0xFF));
    }

    void WriteShort(int16_t value) { WriteUShort(static_cast<uint16_t>(value)); }

    void WriteUInt(uint32_t value) {
        for (size_t index = 0; index < 4; ++index)
            WriteByte(static_cast<uint8_t>((value >> (index * 8)) & 0xFF));
    }

    void WriteInt(int32_t value) { WriteUInt(static_cast<uint32_t>(value)); }

    void WriteULong(uint64_t value) {
        for (size_t index = 0; index < 8; ++index)
            WriteByte(static_cast<uint8_t>((value >> (index * 8)) & 0xFF));
    }

    void WriteFloat(float value) { WriteUInt(std::bit_cast<uint32_t>(value)); }
    void WriteDouble(double value) { WriteULong(std::bit_cast<uint64_t>(value)); }

    void WriteString(std::string_view value) {
        WriteInt(static_cast<int32_t>(value.size()));
        mOut.append(value);
    }

    /// Replays already-encoded bytes, which is how an untouched attribute keeps its exact form.
    void WriteRaw(const unsigned char *data, size_t size) {
        mOut.append(reinterpret_cast<const char *>(data), size);
    }

    void WriteRaw(const std::vector<unsigned char> &data) { WriteRaw(data.data(), data.size()); }

private:
    std::string &mOut;
};

/**
 * @brief The C++ stand-in for IAttributeToken<T>.
 *
 * The reference discovers its tokens by scanning the assembly for implementations of this
 * interface (Attributes.cs:87-115) and dispatches through MethodInfo.Invoke. There is no runtime
 * type discovery here, so the interface becomes a concept every token below satisfies and
 * RbxAttribute dispatches over them with a switch on the type tag.
 */
template <typename Token>
concept IAttributeToken = requires(AttributeReader &reader, AttributeWriter &writer,
                                   const typename Token::ValueType &value) {
    { Token::kAttributeType } -> std::convertible_to<AttributeType>;
    { Token::Read(reader) } -> std::same_as<typename Token::ValueType>;
    { Token::Write(writer, value) } -> std::same_as<void>;
};

// Tokens/String.cs:8-11.
struct StringAttributeToken {
    using ValueType = std::string;
    static constexpr AttributeType kAttributeType = AttributeType::String;

    static ValueType Read(AttributeReader &reader) { return reader.ReadString(); }
    static void Write(AttributeWriter &writer, const ValueType &value) { writer.WriteString(value); }
};

// Tokens/Boolean.cs:9-12.
struct BoolAttributeToken {
    using ValueType = bool;
    static constexpr AttributeType kAttributeType = AttributeType::Bool;

    static ValueType Read(AttributeReader &reader) { return reader.ReadBool(); }
    static void Write(AttributeWriter &writer, const ValueType &value) { writer.WriteBool(value); }
};

// Tokens/Int.cs:9-12.
struct IntAttributeToken {
    using ValueType = int32_t;
    static constexpr AttributeType kAttributeType = AttributeType::Int;

    static ValueType Read(AttributeReader &reader) { return reader.ReadInt(); }
    static void Write(AttributeWriter &writer, const ValueType &value) { writer.WriteInt(value); }
};

// Tokens/Float.cs:9-12.
struct FloatAttributeToken {
    using ValueType = float;
    static constexpr AttributeType kAttributeType = AttributeType::Float;

    static ValueType Read(AttributeReader &reader) { return reader.ReadFloat(); }
    static void Write(AttributeWriter &writer, const ValueType &value) { writer.WriteFloat(value); }
};

// Tokens/Double.cs:9-12.
struct DoubleAttributeToken {
    using ValueType = double;
    static constexpr AttributeType kAttributeType = AttributeType::Double;

    static ValueType Read(AttributeReader &reader) { return reader.ReadDouble(); }
    static void Write(AttributeWriter &writer, const ValueType &value) { writer.WriteDouble(value); }
};

// Tokens/UDim.cs:44-59.
struct UDimAttributeToken {
    using ValueType = DataTypes::UDim;
    static constexpr AttributeType kAttributeType = AttributeType::UDim;

    static ValueType Read(AttributeReader &reader) {
        const float scale = reader.ReadFloat();
        const int32_t offset = reader.ReadInt();
        return DataTypes::UDim(scale, offset);
    }

    static void Write(AttributeWriter &writer, const ValueType &value) {
        writer.WriteFloat(value.Scale);
        writer.WriteInt(value.Offset);
    }
};

// Tokens/UDim2.cs:11-23.
struct UDim2AttributeToken {
    using ValueType = DataTypes::UDim2;
    static constexpr AttributeType kAttributeType = AttributeType::UDim2;

    static ValueType Read(AttributeReader &reader) {
        const DataTypes::UDim x = UDimAttributeToken::Read(reader);
        const DataTypes::UDim y = UDimAttributeToken::Read(reader);
        return DataTypes::UDim2(x, y);
    }

    static void Write(AttributeWriter &writer, const ValueType &value) {
        UDimAttributeToken::Write(writer, value.X);
        UDimAttributeToken::Write(writer, value.Y);
    }
};

// Tokens/BrickColor.cs:13-16.
struct BrickColorAttributeToken {
    using ValueType = DataTypes::BrickColor;
    static constexpr AttributeType kAttributeType = AttributeType::BrickColor;

    static ValueType Read(AttributeReader &reader) { return DataTypes::BrickColor(reader.ReadInt()); }
    static void Write(AttributeWriter &writer, const ValueType &value) { writer.WriteInt(value.Number); }
};

// Tokens/Color3.cs:16-30.
struct Color3AttributeToken {
    using ValueType = DataTypes::Color3;
    static constexpr AttributeType kAttributeType = AttributeType::Color3;

    static ValueType Read(AttributeReader &reader) {
        const float r = reader.ReadFloat();
        const float g = reader.ReadFloat();
        const float b = reader.ReadFloat();
        return DataTypes::Color3(r, g, b);
    }

    static void Write(AttributeWriter &writer, const ValueType &value) {
        writer.WriteFloat(value.R);
        writer.WriteFloat(value.G);
        writer.WriteFloat(value.B);
    }
};

// Tokens/Vector2.cs:49-61.
struct Vector2AttributeToken {
    using ValueType = DataTypes::Vector2;
    static constexpr AttributeType kAttributeType = AttributeType::Vector2;

    static ValueType Read(AttributeReader &reader) {
        const float x = reader.ReadFloat();
        const float y = reader.ReadFloat();
        return DataTypes::Vector2(x, y);
    }

    static void Write(AttributeWriter &writer, const ValueType &value) {
        writer.WriteFloat(value.X);
        writer.WriteFloat(value.Y);
    }
};

// Tokens/Vector3.cs:52-66.
struct Vector3AttributeToken {
    using ValueType = DataTypes::Vector3;
    static constexpr AttributeType kAttributeType = AttributeType::Vector3;

    static ValueType Read(AttributeReader &reader) {
        const float x = reader.ReadFloat();
        const float y = reader.ReadFloat();
        const float z = reader.ReadFloat();
        return DataTypes::Vector3(x, y, z);
    }

    static void Write(AttributeWriter &writer, const ValueType &value) {
        writer.WriteFloat(value.X);
        writer.WriteFloat(value.Y);
        writer.WriteFloat(value.Z);
    }
};

// Tokens/CFrame.cs:80-122.
struct CFrameAttributeToken {
    using ValueType = DataTypes::CFrame;
    static constexpr AttributeType kAttributeType = AttributeType::CFrame;

    static ValueType Read(AttributeReader &reader) {
        const DataTypes::Vector3 position = Vector3AttributeToken::Read(reader);
        const uint8_t orientId = reader.ReadByte();

        // A non-zero id is an axis-aligned rotation packed into the byte; zero means the whole
        // 3x3 matrix follows. The reference forms this as "FromOrientId(id - 1) + pos", and its
        // CFrame + Vector3 (DataTypes/CFrame.cs:160-170) only translates, so the rotation half is
        // untouched either way -- the ported CFrame has no such operator, hence the direct write.
        std::array<float, 12> components {};
        if (orientId > 0) {
            components = DataTypes::CFrame::FromOrientId(orientId - 1).Components;
        } else {
            for (size_t index = 3; index < 12; ++index)
                components[index] = reader.ReadFloat();
        }

        components[0] = position.X;
        components[1] = position.Y;
        components[2] = position.Z;
        return DataTypes::CFrame(components);
    }

    static void Write(AttributeWriter &writer, const ValueType &value) {
        Vector3AttributeToken::Write(writer, value.Position());

        const int orientId = value.GetOrientId();
        writer.WriteByte(static_cast<uint8_t>(orientId + 1));

        if (orientId == -1)
            for (size_t index = 3; index < 12; ++index)
                writer.WriteFloat(value.Components[index]);
    }
};

// Tokens/Enum.cs:64-89.
struct EnumAttributeToken {
    using ValueType = RbxAttributeEnum;
    static constexpr AttributeType kAttributeType = AttributeType::Enum;

    static ValueType Read(AttributeReader &reader) {
        std::string name = reader.ReadString();
        const uint32_t value = reader.ReadUInt();
        return RbxAttributeEnum(std::move(name), value);
    }

    static void Write(AttributeWriter &writer, const ValueType &value) {
        writer.WriteString(value.Name);
        writer.WriteUInt(value.Value);
    }
};

// Tokens/NumberSequence.cs:52-79.
struct NumberSequenceAttributeToken {
    using ValueType = DataTypes::NumberSequence;
    static constexpr AttributeType kAttributeType = AttributeType::NumberSequence;

    // envelope, time, value -- three floats, in that order.
    static constexpr size_t kKeypointSize = 12;

    static ValueType Read(AttributeReader &reader) {
        const int32_t numKeys = reader.ReadInt();
        if (reader.Failed() || numKeys < 0 ||
            static_cast<uint64_t>(numKeys) * kKeypointSize > reader.Remaining()) {
            reader.Fail();
            return DataTypes::NumberSequence(std::vector<DataTypes::NumberSequenceKeypoint> {});
        }

        std::vector<DataTypes::NumberSequenceKeypoint> keypoints;
        keypoints.reserve(static_cast<size_t>(numKeys));
        for (int32_t index = 0; index < numKeys; ++index) {
            // NumberSequence.cs:59 reads the envelope with ReadInt while NumberSequence.cs:75
            // writes it with WriteFloat, so a sequence with a non-zero envelope changes value on
            // every load/save there. Read matches Write here; the wire field is a float.
            const float envelope = reader.ReadFloat();
            const float time = reader.ReadFloat();
            const float value = reader.ReadFloat();
            keypoints.emplace_back(time, value, envelope);
        }

        return DataTypes::NumberSequence(std::move(keypoints));
    }

    static void Write(AttributeWriter &writer, const ValueType &value) {
        writer.WriteInt(static_cast<int32_t>(value.Keypoints.size()));
        for (const DataTypes::NumberSequenceKeypoint &keypoint : value.Keypoints) {
            writer.WriteFloat(keypoint.Envelope);
            writer.WriteFloat(keypoint.Time);
            writer.WriteFloat(keypoint.Value);
        }
    }
};

// Tokens/ColorSequence.cs:55-83.
struct ColorSequenceAttributeToken {
    using ValueType = DataTypes::ColorSequence;
    static constexpr AttributeType kAttributeType = AttributeType::ColorSequence;

    // int32 envelope, float time, three float colour channels.
    static constexpr size_t kKeypointSize = 20;

    static ValueType Read(AttributeReader &reader) {
        const int32_t numKeys = reader.ReadInt();
        if (reader.Failed() || numKeys < 0 ||
            static_cast<uint64_t>(numKeys) * kKeypointSize > reader.Remaining()) {
            reader.Fail();
            return DataTypes::ColorSequence(std::vector<DataTypes::ColorSequenceKeypoint> {});
        }

        std::vector<DataTypes::ColorSequenceKeypoint> keypoints;
        keypoints.reserve(static_cast<size_t>(numKeys));
        for (int32_t index = 0; index < numKeys; ++index) {
            const int32_t envelope = reader.ReadInt();
            const float time = reader.ReadFloat();
            const DataTypes::Color3 value = Color3AttributeToken::Read(reader);
            keypoints.emplace_back(time, value, envelope);
        }

        return DataTypes::ColorSequence(std::move(keypoints));
    }

    static void Write(AttributeWriter &writer, const ValueType &value) {
        writer.WriteInt(static_cast<int32_t>(value.Keypoints.size()));
        for (const DataTypes::ColorSequenceKeypoint &keypoint : value.Keypoints) {
            writer.WriteInt(keypoint.Envelope);
            writer.WriteFloat(keypoint.Time);
            Color3AttributeToken::Write(writer, keypoint.Value);
        }
    }
};

// Tokens/NumberRange.cs:43-55.
struct NumberRangeAttributeToken {
    using ValueType = DataTypes::NumberRange;
    static constexpr AttributeType kAttributeType = AttributeType::NumberRange;

    static ValueType Read(AttributeReader &reader) {
        const float min = reader.ReadFloat();
        const float max = reader.ReadFloat();
        return DataTypes::NumberRange(min, max);
    }

    static void Write(AttributeWriter &writer, const ValueType &value) {
        writer.WriteFloat(value.Min);
        writer.WriteFloat(value.Max);
    }
};

// Tokens/Rect.cs:54-66.
struct RectAttributeToken {
    using ValueType = DataTypes::Rect;
    static constexpr AttributeType kAttributeType = AttributeType::Rect;

    static ValueType Read(AttributeReader &reader) {
        const DataTypes::Vector2 min = Vector2AttributeToken::Read(reader);
        const DataTypes::Vector2 max = Vector2AttributeToken::Read(reader);
        return DataTypes::Rect(min, max);
    }

    static void Write(AttributeWriter &writer, const ValueType &value) {
        Vector2AttributeToken::Write(writer, value.Min);
        Vector2AttributeToken::Write(writer, value.Max);
    }
};

// Tokens/Font.cs:79-104.
struct FontFaceAttributeToken {
    using ValueType = DataTypes::FontFace;
    static constexpr AttributeType kAttributeType = AttributeType::FontFace;

    static ValueType Read(AttributeReader &reader) {
        // Font.cs:90-104 writes (int16)Weight then (byte)Style, but Font.cs:79-88 hands the int16
        // to the FontFace constructor's style parameter and the byte to its weight parameter --
        // every Font attribute would come back with the two swapped. Read matches Write here.
        const auto weight = static_cast<DataTypes::FontWeight>(reader.ReadUShort());
        const auto style = static_cast<DataTypes::FontStyle>(reader.ReadByte());
        std::string family = reader.ReadString();
        std::string cachedFaceId = reader.ReadString();
        return DataTypes::FontFace(std::move(family), weight, style, std::move(cachedFaceId));
    }

    static void Write(AttributeWriter &writer, const ValueType &value) {
        writer.WriteUShort(static_cast<uint16_t>(value.Weight));
        writer.WriteByte(static_cast<uint8_t>(value.Style));
        writer.WriteString(value.Family);
        writer.WriteString(value.CachedFaceId);
    }
};

/**
 * @brief Maps a C++ value type onto the token that serializes it.
 *
 * This is the SupportedTypes half of Attributes.cs:52 -- the reference builds it at static
 * construction from the generic argument of each IAttributeToken<T>; here the mapping is spelled
 * out because it has to be known at compile time. A type with no specialization is a type
 * attributes do not support, which is what SetAttribute reports on.
 */
template <typename T>
struct AttributeTokenOf {};

template <> struct AttributeTokenOf<std::string> { using Token = StringAttributeToken; };
template <> struct AttributeTokenOf<bool> { using Token = BoolAttributeToken; };
template <> struct AttributeTokenOf<int32_t> { using Token = IntAttributeToken; };
template <> struct AttributeTokenOf<float> { using Token = FloatAttributeToken; };
template <> struct AttributeTokenOf<double> { using Token = DoubleAttributeToken; };
template <> struct AttributeTokenOf<DataTypes::UDim> { using Token = UDimAttributeToken; };
template <> struct AttributeTokenOf<DataTypes::UDim2> { using Token = UDim2AttributeToken; };
template <> struct AttributeTokenOf<DataTypes::BrickColor> { using Token = BrickColorAttributeToken; };
template <> struct AttributeTokenOf<DataTypes::Color3> { using Token = Color3AttributeToken; };
template <> struct AttributeTokenOf<DataTypes::Vector2> { using Token = Vector2AttributeToken; };
template <> struct AttributeTokenOf<DataTypes::Vector3> { using Token = Vector3AttributeToken; };
template <> struct AttributeTokenOf<DataTypes::CFrame> { using Token = CFrameAttributeToken; };
template <> struct AttributeTokenOf<RbxAttributeEnum> { using Token = EnumAttributeToken; };
template <> struct AttributeTokenOf<DataTypes::NumberSequence> { using Token = NumberSequenceAttributeToken; };
template <> struct AttributeTokenOf<DataTypes::ColorSequence> { using Token = ColorSequenceAttributeToken; };
template <> struct AttributeTokenOf<DataTypes::NumberRange> { using Token = NumberRangeAttributeToken; };
template <> struct AttributeTokenOf<DataTypes::Rect> { using Token = RectAttributeToken; };
template <> struct AttributeTokenOf<DataTypes::FontFace> { using Token = FontFaceAttributeToken; };

/**
 * @brief The type an attribute actually stores for a value written as T.
 *
 * C# boxes a string literal as a string; C++ deduces const char *, and a std::any holding a
 * pointer would neither serialize nor compare. Narrowing the string-ish spellings to std::string
 * here keeps SetAttribute("key", "text") doing what it reads like.
 */
template <typename T>
struct AttributeValueMap {
    using Type = T;
};

template <> struct AttributeValueMap<char *> { using Type = std::string; };
template <> struct AttributeValueMap<const char *> { using Type = std::string; };
template <> struct AttributeValueMap<std::string_view> { using Type = std::string; };
template <size_t N> struct AttributeValueMap<char[N]> { using Type = std::string; };
template <size_t N> struct AttributeValueMap<const char[N]> { using Type = std::string; };

template <typename T>
using AttributeValueType = typename AttributeValueMap<std::remove_cvref_t<T>>::Type;

/// Attributes.cs:122-136: whether a value of this type can be stored in an attribute at all.
template <typename T>
concept SupportedAttributeType = requires { typename AttributeTokenOf<AttributeValueType<T>>::Token; };
}
