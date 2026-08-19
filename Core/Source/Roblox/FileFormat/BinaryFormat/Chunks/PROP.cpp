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
// File: PROP.cpp
// Started by: Hattozo
// Started on: 8/18/2025
// Description: Decodes and re-encodes one PROP chunk value column.
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/Chunks/PROP.h>
#include <NoobWarrior/Roblox/FileFormat/Utility/MaterialInfo.h>

#include <optional>

using namespace NoobWarrior::Roblox;
using namespace NoobWarrior::Roblox::DataTypes;

namespace {
// Plain (unrotated) interleaved columns. Enum, BrickColor, SharedString and SecurityCapabilities
// store their words verbatim rather than zigzag-encoded.
std::vector<uint32_t> ReadPlainUInt32(BinaryRobloxFileReader &reader, size_t count) {
    std::vector<unsigned char> raw;
    std::vector<uint32_t> values;
    if (!reader.ReadInterleaved(count, sizeof(uint32_t), raw))
        return values;
    values.resize(count);
    for (size_t index = 0; index < count; ++index)
        std::memcpy(&values[index], raw.data() + index * sizeof(uint32_t), sizeof(uint32_t));
    return values;
}

void WritePlainUInt32(BinaryRobloxFileWriter &writer, const std::vector<uint32_t> &values) {
    std::vector<unsigned char> raw(values.size() * sizeof(uint32_t));
    for (size_t index = 0; index < values.size(); ++index)
        std::memcpy(raw.data() + index * sizeof(uint32_t), &values[index], sizeof(uint32_t));
    writer.WriteInterleaved(raw, values.size(), sizeof(uint32_t));
}

std::vector<uint64_t> ReadPlainUInt64(BinaryRobloxFileReader &reader, size_t count) {
    std::vector<unsigned char> raw;
    std::vector<uint64_t> values;
    if (!reader.ReadInterleaved(count, sizeof(uint64_t), raw))
        return values;
    values.resize(count);
    for (size_t index = 0; index < count; ++index)
        std::memcpy(&values[index], raw.data() + index * sizeof(uint64_t), sizeof(uint64_t));
    return values;
}

void WritePlainUInt64(BinaryRobloxFileWriter &writer, const std::vector<uint64_t> &values) {
    std::vector<unsigned char> raw(values.size() * sizeof(uint64_t));
    for (size_t index = 0; index < values.size(); ++index)
        std::memcpy(raw.data() + index * sizeof(uint64_t), &values[index], sizeof(uint64_t));
    writer.WriteInterleaved(raw, values.size(), sizeof(uint64_t));
}

template<typename T>
const T *Get(const std::any &value) {
    return std::any_cast<T>(&value);
}

// Reads the shared "float column per component" shape used by Color3, Vector2/3, UDim, Rect
// and CFrame positions.
struct FloatColumns {
    std::vector<std::vector<float>> Columns;

    bool Read(BinaryRobloxFileReader &reader, size_t count, size_t columns) {
        Columns.clear();
        for (size_t index = 0; index < columns; ++index) {
            Columns.push_back(reader.ReadFloats(count));
            if (reader.Failed())
                return false;
        }
        return true;
    }
};
// A class chunk declares one type per property, but its values can arrive in more than one
// representation: BuildTables groups instances by ClassName, so a place's own objects and a
// mounted plugin model end up sharing a column even when their source files stored the property
// differently. Writing the wrong representation produces a file Studio refuses to open, so every
// value is normalised to the column's declared type before encoding.
std::any CoerceToColumnType(const std::any &value, PropertyType type) {
    const auto text = [&]() -> std::optional<std::string> {
        if (const auto *s = Get<std::string>(value)) return *s;
        if (const auto *p = Get<ProtectedString>(value)) return p->ToString();
        if (const auto *c = Get<Content>(value)) return c->Uri;
        if (const auto *c = Get<ContentId>(value)) return c->Uri;
        return std::nullopt;
    };
    const auto integral = [&]() -> std::optional<int64_t> {
        if (const auto *v = Get<int32_t>(value)) return *v;
        if (const auto *v = Get<int64_t>(value)) return *v;
        if (const auto *v = Get<uint32_t>(value)) return static_cast<int64_t>(*v);
        return std::nullopt;
    };
    const auto real = [&]() -> std::optional<double> {
        if (const auto *v = Get<float>(value)) return *v;
        if (const auto *v = Get<double>(value)) return *v;
        return std::nullopt;
    };

    switch (type) {
    case PropertyType::String:
        if (Get<std::string>(value)) return value;
        if (const auto s = text()) return *s;
        break;
    case PropertyType::ProtectedString:
        if (Get<ProtectedString>(value)) return value;
        if (const auto s = text()) return ProtectedString(*s);
        break;
    case PropertyType::Content:
        if (Get<Content>(value)) return value;
        if (const auto s = text()) return s->empty() ? Content() : Content(*s);
        break;
    case PropertyType::Int:
        if (Get<int32_t>(value)) return value;
        if (const auto v = integral()) return static_cast<int32_t>(*v);
        break;
    case PropertyType::Int64:
        if (Get<int64_t>(value)) return value;
        if (const auto v = integral()) return *v;
        break;
    case PropertyType::BrickColor:
        if (Get<int32_t>(value)) return value;
        if (const auto v = integral()) return static_cast<int32_t>(*v);
        break;
    case PropertyType::Enum:
    case PropertyType::SharedString:
        if (Get<uint32_t>(value)) return value;
        if (const auto v = integral()) return static_cast<uint32_t>(*v);
        break;
    case PropertyType::Float:
        if (Get<float>(value)) return value;
        if (const auto v = real()) return static_cast<float>(*v);
        break;
    case PropertyType::Double:
        if (Get<double>(value)) return value;
        if (const auto v = real()) return *v;
        break;
    case PropertyType::Color3:
        if (Get<Color3>(value)) return value;
        if (const auto *packed = Get<Color3uint8>(value)) return packed->ToColor3();
        break;
    case PropertyType::Color3uint8:
        if (Get<Color3uint8>(value)) return value;
        if (const auto *floats = Get<Color3>(value)) {
            const auto clamp = [](float c) {
                const float scaled = c * 255.0f + 0.5f;
                return static_cast<uint8_t>(scaled < 0.0f ? 0.0f
                    : (scaled > 255.0f ? 255.0f : scaled));
            };
            return Color3uint8(clamp(floats->R), clamp(floats->G), clamp(floats->B));
        }
        break;
    case PropertyType::Vector3:
        if (Get<Vector3>(value)) return value;
        if (const auto *v = Get<Vector3int16>(value))
            return Vector3(v->X, v->Y, v->Z);
        break;
    case PropertyType::Vector3int16:
        if (Get<Vector3int16>(value)) return value;
        if (const auto *v = Get<Vector3>(value)) {
            return Vector3int16(static_cast<int16_t>(v->X), static_cast<int16_t>(v->Y),
                                static_cast<int16_t>(v->Z));
        }
        break;
    default:
        break;
    }
    return value;
}
} // namespace

bool PROP::IsSupported(PropertyType type) {
    switch (type) {
    case PropertyType::String:      case PropertyType::Bool:
    case PropertyType::Int:         case PropertyType::Float:
    case PropertyType::Double:      case PropertyType::UDim:
    case PropertyType::UDim2:       case PropertyType::Ray:
    case PropertyType::Faces:       case PropertyType::Axes:
    case PropertyType::BrickColor:  case PropertyType::Color3:
    case PropertyType::Vector2:     case PropertyType::Vector3:
    case PropertyType::CFrame:      case PropertyType::Quaternion:
    case PropertyType::Enum:        case PropertyType::Ref:
    case PropertyType::Vector3int16: case PropertyType::NumberSequence:
    case PropertyType::ColorSequence: case PropertyType::NumberRange:
    case PropertyType::Rect:        case PropertyType::PhysicalProperties:
    case PropertyType::Color3uint8: case PropertyType::Int64:
    case PropertyType::SharedString: case PropertyType::ProtectedString:
    case PropertyType::OptionalCFrame: case PropertyType::UniqueId:
    case PropertyType::FontFace:    case PropertyType::SecurityCapabilities:
    case PropertyType::Content:
        return true;
    default:
        return false;
    }
}

bool PROP::Read(BinaryRobloxFileReader &reader, PropertyType type, size_t count,
                         PROP::ValueColumn &column) {
    column = {};
    column.Type = type;
    column.Values.clear();
    column.Values.reserve(count);

    const auto fail = [&]() {
        column.Values.clear();
        return false;
    };

    switch (type) {
    case PropertyType::String: {
        for (size_t index = 0; index < count; ++index) {
            const std::vector<unsigned char> raw = reader.ReadRawString();
            if (reader.Failed())
                return fail();
            column.Values.emplace_back(std::string(raw.begin(), raw.end()));
        }
        break;
    }
    case PropertyType::ProtectedString: {
        for (size_t index = 0; index < count; ++index) {
            std::vector<unsigned char> raw = reader.ReadRawString();
            if (reader.Failed())
                return fail();
            column.Values.emplace_back(ProtectedString(std::move(raw)));
        }
        break;
    }
    case PropertyType::Bool: {
        for (size_t index = 0; index < count; ++index)
            column.Values.emplace_back(reader.ReadByte() != 0);
        break;
    }
    case PropertyType::Int: {
        const std::vector<int32_t> values = reader.ReadInts(count);
        if (reader.Failed())
            return fail();
        for (int32_t value : values)
            column.Values.emplace_back(value);
        break;
    }
    case PropertyType::Float: {
        const std::vector<float> values = reader.ReadFloats(count);
        if (reader.Failed())
            return fail();
        for (float value : values)
            column.Values.emplace_back(value);
        break;
    }
    case PropertyType::Double: {
        for (size_t index = 0; index < count; ++index)
            column.Values.emplace_back(reader.ReadDouble());
        break;
    }
    case PropertyType::Int64: {
        const std::vector<int64_t> values = reader.ReadLongs(count);
        if (reader.Failed())
            return fail();
        for (int64_t value : values)
            column.Values.emplace_back(value);
        break;
    }
    case PropertyType::UDim: {
        const std::vector<float> scales = reader.ReadFloats(count);
        const std::vector<int32_t> offsets = reader.ReadInts(count);
        if (reader.Failed())
            return fail();
        for (size_t index = 0; index < count; ++index)
            column.Values.emplace_back(UDim(scales[index], offsets[index]));
        break;
    }
    case PropertyType::UDim2: {
        const std::vector<float> scaleX = reader.ReadFloats(count);
        const std::vector<float> scaleY = reader.ReadFloats(count);
        const std::vector<int32_t> offsetX = reader.ReadInts(count);
        const std::vector<int32_t> offsetY = reader.ReadInts(count);
        if (reader.Failed())
            return fail();
        for (size_t index = 0; index < count; ++index) {
            column.Values.emplace_back(UDim2(scaleX[index], offsetX[index],
                                             scaleY[index], offsetY[index]));
        }
        break;
    }
    case PropertyType::Ray: {
        for (size_t index = 0; index < count; ++index) {
            const float ox = reader.ReadFloat(), oy = reader.ReadFloat(), oz = reader.ReadFloat();
            const float dx = reader.ReadFloat(), dy = reader.ReadFloat(), dz = reader.ReadFloat();
            if (reader.Failed())
                return fail();
            column.Values.emplace_back(Ray({ox, oy, oz}, {dx, dy, dz}));
        }
        break;
    }
    case PropertyType::Faces: {
        for (size_t index = 0; index < count; ++index)
            column.Values.emplace_back(Faces(reader.ReadByte()));
        break;
    }
    case PropertyType::Axes: {
        for (size_t index = 0; index < count; ++index)
            column.Values.emplace_back(Axes(reader.ReadByte()));
        break;
    }
    case PropertyType::BrickColor: {
        const std::vector<uint32_t> values = ReadPlainUInt32(reader, count);
        if (reader.Failed())
            return fail();
        for (uint32_t value : values)
            column.Values.emplace_back(static_cast<int32_t>(value));
        break;
    }
    case PropertyType::Enum: {
        const std::vector<uint32_t> values = ReadPlainUInt32(reader, count);
        if (reader.Failed())
            return fail();
        for (uint32_t value : values)
            column.Values.emplace_back(value);
        break;
    }
    case PropertyType::SharedString: {
        const std::vector<uint32_t> values = ReadPlainUInt32(reader, count);
        if (reader.Failed())
            return fail();
        for (uint32_t value : values)
            column.Values.emplace_back(value);
        break;
    }
    case PropertyType::SecurityCapabilities: {
        const std::vector<uint64_t> values = ReadPlainUInt64(reader, count);
        if (reader.Failed())
            return fail();
        for (uint64_t value : values)
            column.Values.emplace_back(SecurityCapabilities(value));
        break;
    }
    case PropertyType::Ref: {
        const std::vector<int32_t> values = reader.ReadReferents(count);
        if (reader.Failed())
            return fail();
        for (int32_t value : values)
            column.Values.emplace_back(value);
        break;
    }
    case PropertyType::Color3: {
        FloatColumns columns;
        if (!columns.Read(reader, count, 3))
            return fail();
        for (size_t index = 0; index < count; ++index) {
            column.Values.emplace_back(Color3(columns.Columns[0][index],
                                              columns.Columns[1][index],
                                              columns.Columns[2][index]));
        }
        break;
    }
    case PropertyType::Vector2: {
        FloatColumns columns;
        if (!columns.Read(reader, count, 2))
            return fail();
        for (size_t index = 0; index < count; ++index)
            column.Values.emplace_back(Vector2(columns.Columns[0][index], columns.Columns[1][index]));
        break;
    }
    case PropertyType::Vector3: {
        FloatColumns columns;
        if (!columns.Read(reader, count, 3))
            return fail();
        for (size_t index = 0; index < count; ++index) {
            column.Values.emplace_back(Vector3(columns.Columns[0][index],
                                               columns.Columns[1][index],
                                               columns.Columns[2][index]));
        }
        break;
    }
    case PropertyType::Rect: {
        FloatColumns columns;
        if (!columns.Read(reader, count, 4))
            return fail();
        for (size_t index = 0; index < count; ++index) {
            column.Values.emplace_back(Rect({columns.Columns[0][index], columns.Columns[1][index]},
                                            {columns.Columns[2][index], columns.Columns[3][index]}));
        }
        break;
    }
    case PropertyType::Vector3int16: {
        for (size_t index = 0; index < count; ++index) {
            int16_t x = 0, y = 0, z = 0;
            reader.ReadBytes(&x, sizeof(x));
            reader.ReadBytes(&y, sizeof(y));
            reader.ReadBytes(&z, sizeof(z));
            if (reader.Failed())
                return fail();
            column.Values.emplace_back(Vector3int16(x, y, z));
        }
        break;
    }
    case PropertyType::NumberRange: {
        for (size_t index = 0; index < count; ++index) {
            const float min = reader.ReadFloat();
            const float max = reader.ReadFloat();
            if (reader.Failed())
                return fail();
            column.Values.emplace_back(NumberRange(min, max));
        }
        break;
    }
    case PropertyType::NumberSequence: {
        for (size_t index = 0; index < count; ++index) {
            const int32_t keys = reader.ReadInt32();
            if (reader.Failed() || keys < 0)
                return fail();
            std::vector<NumberSequenceKeypoint> keypoints;
            keypoints.reserve(static_cast<size_t>(keys));
            for (int32_t key = 0; key < keys; ++key) {
                const float time = reader.ReadFloat();
                const float value = reader.ReadFloat();
                const float envelope = reader.ReadFloat();
                if (reader.Failed())
                    return fail();
                keypoints.emplace_back(time, value, envelope);
            }
            column.Values.emplace_back(NumberSequence(std::move(keypoints)));
        }
        break;
    }
    case PropertyType::ColorSequence: {
        for (size_t index = 0; index < count; ++index) {
            const int32_t keys = reader.ReadInt32();
            if (reader.Failed() || keys < 0)
                return fail();
            std::vector<ColorSequenceKeypoint> keypoints;
            keypoints.reserve(static_cast<size_t>(keys));
            for (int32_t key = 0; key < keys; ++key) {
                const float time = reader.ReadFloat();
                const float r = reader.ReadFloat();
                const float g = reader.ReadFloat();
                const float b = reader.ReadFloat();
                // Envelope occupies four bytes but is serialized as an int32.
                const int32_t envelope = reader.ReadInt32();
                if (reader.Failed())
                    return fail();
                keypoints.emplace_back(time, Color3(r, g, b), envelope);
            }
            column.Values.emplace_back(ColorSequence(std::move(keypoints)));
        }
        break;
    }
    case PropertyType::PhysicalProperties: {
        column.RawFlags.assign(count, 0);
        for (size_t index = 0; index < count; ++index) {
            const uint8_t flags = reader.ReadByte();
            if (reader.Failed())
                return fail();
            column.RawFlags[index] = flags;
            if (!HasFlag(flags, MaterialBitFlags::CustomPhysics)) {
                column.Values.emplace_back(std::optional<PhysicalProperties>());
                continue;
            }
            PhysicalProperties value;
            value.CustomPhysics = true;
            value.Flags = flags;
            value.Density = reader.ReadFloat();
            value.Friction = reader.ReadFloat();
            value.Elasticity = reader.ReadFloat();
            value.FrictionWeight = reader.ReadFloat();
            value.ElasticityWeight = reader.ReadFloat();
            if (HasFlag(flags, MaterialBitFlags::AcousticAbsorption))
                value.AcousticAbsorption = reader.ReadFloat();
            if (reader.Failed())
                return fail();
            column.Values.emplace_back(std::optional<PhysicalProperties>(value));
        }
        break;
    }
    case PropertyType::Color3uint8: {
        std::vector<unsigned char> red(count), green(count), blue(count);
        if (!reader.ReadBytes(red.data(), count) || !reader.ReadBytes(green.data(), count) ||
            !reader.ReadBytes(blue.data(), count)) {
            return fail();
        }
        for (size_t index = 0; index < count; ++index)
            column.Values.emplace_back(Color3uint8(red[index], green[index], blue[index]));
        break;
    }
    case PropertyType::UniqueId: {
        std::vector<unsigned char> raw;
        if (!reader.ReadInterleaved(count, 16, raw))
            return fail();
        for (size_t index = 0; index < count; ++index) {
            const unsigned char *entry = raw.data() + index * 16;
            uint64_t random = 0;
            uint32_t time = 0;
            uint32_t counter = 0;
            std::memcpy(&random, entry, sizeof(random));
            std::memcpy(&time, entry + 8, sizeof(time));
            std::memcpy(&counter, entry + 12, sizeof(counter));
            column.Values.emplace_back(
                UniqueId(BinaryRobloxFileReader::RotateInt64(random), time, counter));
        }
        break;
    }
    case PropertyType::FontFace: {
        for (size_t index = 0; index < count; ++index) {
            const std::string family = reader.ReadString();
            uint16_t weight = 0;
            reader.ReadBytes(&weight, sizeof(weight));
            const uint8_t style = reader.ReadByte();
            const std::string cachedFaceId = reader.ReadString();
            if (reader.Failed())
                return fail();
            column.Values.emplace_back(FontFace(family, static_cast<FontWeight>(weight),
                                                static_cast<FontStyle>(style), cachedFaceId));
        }
        break;
    }
    case PropertyType::CFrame:
    case PropertyType::Quaternion:
    case PropertyType::OptionalCFrame: {
        if (type == PropertyType::OptionalCFrame) {
            if (reader.ReadByte() != static_cast<uint8_t>(PropertyType::CFrame))
                return fail();
        }

        std::vector<std::array<float, 9>> rotations(count);
        column.OrientIds.assign(count, 0);
        for (size_t index = 0; index < count; ++index) {
            const uint8_t orientId = reader.ReadByte();
            if (reader.Failed())
                return fail();
            column.OrientIds[index] = orientId;
            if (orientId > 0) {
                const CFrame packed = CFrame::FromOrientId(orientId - 1);
                for (size_t element = 0; element < 9; ++element)
                    rotations[index][element] = packed.Components[element + 3];
            } else if (type == PropertyType::Quaternion) {
                // Quaternion columns are not produced by Roblox's own serializer, but the format
                // permits them; store the raw components so a re-encode is byte-identical.
                for (size_t element = 0; element < 4; ++element)
                    rotations[index][element] = reader.ReadFloat();
                if (reader.Failed())
                    return fail();
            } else {
                for (size_t element = 0; element < 9; ++element)
                    rotations[index][element] = reader.ReadFloat();
                if (reader.Failed())
                    return fail();
            }
        }

        FloatColumns positions;
        if (!positions.Read(reader, count, 3))
            return fail();

        std::vector<uint8_t> present(count, 1);
        if (type == PropertyType::OptionalCFrame) {
            if (reader.ReadByte() != static_cast<uint8_t>(PropertyType::Bool))
                return fail();
            for (size_t index = 0; index < count; ++index)
                present[index] = reader.ReadByte();
            if (reader.Failed())
                return fail();
        }

        for (size_t index = 0; index < count; ++index) {
            std::array<float, 12> components {};
            components[0] = positions.Columns[0][index];
            components[1] = positions.Columns[1][index];
            components[2] = positions.Columns[2][index];
            for (size_t element = 0; element < 9; ++element)
                components[element + 3] = rotations[index][element];

            if (type == PropertyType::OptionalCFrame) {
                column.Values.emplace_back(present[index] != 0
                    ? std::optional<CFrame>(CFrame(components))
                    : std::optional<CFrame>());
            } else {
                column.Values.emplace_back(CFrame(components));
            }
        }
        break;
    }
    case PropertyType::Content: {
        const std::vector<int32_t> sourceTypes = reader.ReadInts(count);
        if (reader.Failed())
            return fail();

        const uint32_t uriCount = reader.ReadUInt32();
        if (reader.Failed())
            return fail();
        std::vector<std::string> uris;
        uris.reserve(uriCount);
        for (uint32_t index = 0; index < uriCount; ++index) {
            uris.push_back(reader.ReadString());
            if (reader.Failed())
                return fail();
        }

        const int32_t objectCount = reader.ReadInt32();
        if (reader.Failed() || objectCount < 0)
            return fail();
        const std::vector<int32_t> objectIds =
            reader.ReadReferents(static_cast<size_t>(objectCount));

        const int32_t externalCount = reader.ReadInt32();
        if (reader.Failed() || externalCount < 0)
            return fail();
        column.ExternalContentObjects =
            reader.ReadReferents(static_cast<size_t>(externalCount));
        if (reader.Failed())
            return fail();

        size_t nextUri = 0;
        size_t nextObject = 0;
        for (size_t index = 0; index < count; ++index) {
            Content value;
            value.SourceType = static_cast<ContentSourceType>(sourceTypes[index]);
            if (value.SourceType == ContentSourceType::Uri) {
                if (nextUri >= uris.size())
                    return fail();
                value.Uri = uris[nextUri++];
            } else if (value.SourceType == ContentSourceType::Object) {
                if (nextObject >= objectIds.size())
                    return fail();
                value.ObjectReferent = objectIds[nextObject++];
            }
            column.Values.emplace_back(std::move(value));
        }
        break;
    }
    default:
        return fail();
    }

    return !reader.Failed();
}

bool PROP::Write(BinaryRobloxFileWriter &writer, const PROP::ValueColumn &column) {
    const size_t count = column.Values.size();
    std::vector<std::any> normalised;
    normalised.reserve(count);
    for (const std::any &value : column.Values)
        normalised.push_back(CoerceToColumnType(value, column.Type));


    switch (column.Type) {
    // String and ProtectedString are byte-identical on the wire, and a single column can end up
    // holding both representations: a file may store Script.Source as either, while an appended
    // script always carries a ProtectedString. Accept whichever a value happens to hold.
    case PropertyType::String:
    case PropertyType::ProtectedString: {
        for (const std::any &value : normalised) {
            if (const auto *text = Get<std::string>(value)) {
                writer.WriteString(*text);
            } else if (const auto *content = Get<Content>(value)) {
                writer.WriteString(content->Uri);
            } else if (const auto *protectedText = Get<ProtectedString>(value)) {
                writer.WriteRawString(protectedText->RawBuffer);
            } else {
                return false;
            }
        }
        break;
    }
    case PropertyType::Bool: {
        for (const std::any &value : normalised) {
            const auto *flag = Get<bool>(value);
            if (flag == nullptr)
                return false;
            writer.WriteByte(*flag ? 1 : 0);
        }
        break;
    }
    case PropertyType::Int: {
        std::vector<int32_t> values;
        normalised.reserve(count);
        for (const std::any &value : normalised) {
            const auto *number = Get<int32_t>(value);
            if (number == nullptr)
                return false;
            values.push_back(*number);
        }
        writer.WriteInts(values);
        break;
    }
    case PropertyType::Float: {
        std::vector<float> values;
        normalised.reserve(count);
        for (const std::any &value : normalised) {
            const auto *number = Get<float>(value);
            if (number == nullptr)
                return false;
            values.push_back(*number);
        }
        writer.WriteFloats(values);
        break;
    }
    case PropertyType::Double: {
        for (const std::any &value : normalised) {
            const auto *number = Get<double>(value);
            if (number == nullptr)
                return false;
            writer.WriteDouble(*number);
        }
        break;
    }
    case PropertyType::Int64: {
        std::vector<int64_t> values;
        normalised.reserve(count);
        for (const std::any &value : normalised) {
            const auto *number = Get<int64_t>(value);
            if (number == nullptr)
                return false;
            values.push_back(*number);
        }
        writer.WriteLongs(values);
        break;
    }
    case PropertyType::UDim: {
        std::vector<float> scales;
        std::vector<int32_t> offsets;
        for (const std::any &value : normalised) {
            const auto *udim = Get<UDim>(value);
            if (udim == nullptr)
                return false;
            scales.push_back(udim->Scale);
            offsets.push_back(udim->Offset);
        }
        writer.WriteFloats(scales);
        writer.WriteInts(offsets);
        break;
    }
    case PropertyType::UDim2: {
        std::vector<float> scaleX, scaleY;
        std::vector<int32_t> offsetX, offsetY;
        for (const std::any &value : normalised) {
            const auto *udim = Get<UDim2>(value);
            if (udim == nullptr)
                return false;
            scaleX.push_back(udim->X.Scale);
            scaleY.push_back(udim->Y.Scale);
            offsetX.push_back(udim->X.Offset);
            offsetY.push_back(udim->Y.Offset);
        }
        writer.WriteFloats(scaleX);
        writer.WriteFloats(scaleY);
        writer.WriteInts(offsetX);
        writer.WriteInts(offsetY);
        break;
    }
    case PropertyType::Ray: {
        for (const std::any &value : normalised) {
            const auto *ray = Get<Ray>(value);
            if (ray == nullptr)
                return false;
            writer.WriteFloat(ray->Origin.X);
            writer.WriteFloat(ray->Origin.Y);
            writer.WriteFloat(ray->Origin.Z);
            writer.WriteFloat(ray->Direction.X);
            writer.WriteFloat(ray->Direction.Y);
            writer.WriteFloat(ray->Direction.Z);
        }
        break;
    }
    case PropertyType::Faces: {
        for (const std::any &value : normalised) {
            const auto *faces = Get<Faces>(value);
            if (faces == nullptr)
                return false;
            writer.WriteByte(faces->Flags());
        }
        break;
    }
    case PropertyType::Axes: {
        for (const std::any &value : normalised) {
            const auto *axes = Get<Axes>(value);
            if (axes == nullptr)
                return false;
            writer.WriteByte(axes->Flags());
        }
        break;
    }
    case PropertyType::BrickColor: {
        std::vector<uint32_t> values;
        normalised.reserve(count);
        for (const std::any &value : normalised) {
            const auto *number = Get<int32_t>(value);
            if (number == nullptr)
                return false;
            values.push_back(static_cast<uint32_t>(*number));
        }
        WritePlainUInt32(writer, values);
        break;
    }
    case PropertyType::Enum:
    case PropertyType::SharedString: {
        std::vector<uint32_t> values;
        normalised.reserve(count);
        for (const std::any &value : normalised) {
            const auto *number = Get<uint32_t>(value);
            if (number == nullptr)
                return false;
            values.push_back(*number);
        }
        WritePlainUInt32(writer, values);
        break;
    }
    case PropertyType::SecurityCapabilities: {
        std::vector<uint64_t> values;
        normalised.reserve(count);
        for (const std::any &value : normalised) {
            const auto *capabilities = Get<SecurityCapabilities>(value);
            if (capabilities == nullptr)
                return false;
            values.push_back(capabilities->Value);
        }
        WritePlainUInt64(writer, values);
        break;
    }
    case PropertyType::Ref: {
        std::vector<int32_t> values;
        normalised.reserve(count);
        for (const std::any &value : normalised) {
            const auto *referent = Get<int32_t>(value);
            if (referent == nullptr)
                return false;
            values.push_back(*referent);
        }
        writer.WriteReferents(values);
        break;
    }
    case PropertyType::Color3: {
        std::vector<float> red, green, blue;
        for (const std::any &value : normalised) {
            Color3 color;
            if (const auto *typed = Get<Color3>(value)) {
                color = *typed;
            } else if (const auto *packed = Get<Color3uint8>(value)) {
                color = packed->ToColor3();
            } else {
                return false;
            }
            red.push_back(color.R);
            green.push_back(color.G);
            blue.push_back(color.B);
        }
        writer.WriteFloats(red);
        writer.WriteFloats(green);
        writer.WriteFloats(blue);
        break;
    }
    case PropertyType::Vector2: {
        std::vector<float> x, y;
        for (const std::any &value : normalised) {
            const auto *vector = Get<Vector2>(value);
            if (vector == nullptr)
                return false;
            x.push_back(vector->X);
            y.push_back(vector->Y);
        }
        writer.WriteFloats(x);
        writer.WriteFloats(y);
        break;
    }
    case PropertyType::Vector3: {
        std::vector<float> x, y, z;
        for (const std::any &value : normalised) {
            const auto *vector = Get<Vector3>(value);
            if (vector == nullptr)
                return false;
            x.push_back(vector->X);
            y.push_back(vector->Y);
            z.push_back(vector->Z);
        }
        writer.WriteFloats(x);
        writer.WriteFloats(y);
        writer.WriteFloats(z);
        break;
    }
    case PropertyType::Rect: {
        std::vector<float> x0, y0, x1, y1;
        for (const std::any &value : normalised) {
            const auto *rect = Get<Rect>(value);
            if (rect == nullptr)
                return false;
            x0.push_back(rect->Min.X);
            y0.push_back(rect->Min.Y);
            x1.push_back(rect->Max.X);
            y1.push_back(rect->Max.Y);
        }
        writer.WriteFloats(x0);
        writer.WriteFloats(y0);
        writer.WriteFloats(x1);
        writer.WriteFloats(y1);
        break;
    }
    case PropertyType::Vector3int16: {
        for (const std::any &value : normalised) {
            const auto *vector = Get<Vector3int16>(value);
            if (vector == nullptr)
                return false;
            writer.WriteBytes(&vector->X, sizeof(vector->X));
            writer.WriteBytes(&vector->Y, sizeof(vector->Y));
            writer.WriteBytes(&vector->Z, sizeof(vector->Z));
        }
        break;
    }
    case PropertyType::NumberRange: {
        for (const std::any &value : normalised) {
            const auto *range = Get<NumberRange>(value);
            if (range == nullptr)
                return false;
            writer.WriteFloat(range->Min);
            writer.WriteFloat(range->Max);
        }
        break;
    }
    case PropertyType::NumberSequence: {
        for (const std::any &value : normalised) {
            const auto *sequence = Get<NumberSequence>(value);
            if (sequence == nullptr)
                return false;
            writer.WriteInt32(static_cast<int32_t>(sequence->Keypoints.size()));
            for (const NumberSequenceKeypoint &keypoint : sequence->Keypoints) {
                writer.WriteFloat(keypoint.Time);
                writer.WriteFloat(keypoint.Value);
                writer.WriteFloat(keypoint.Envelope);
            }
        }
        break;
    }
    case PropertyType::ColorSequence: {
        for (const std::any &value : normalised) {
            const auto *sequence = Get<ColorSequence>(value);
            if (sequence == nullptr)
                return false;
            writer.WriteInt32(static_cast<int32_t>(sequence->Keypoints.size()));
            for (const ColorSequenceKeypoint &keypoint : sequence->Keypoints) {
                writer.WriteFloat(keypoint.Time);
                writer.WriteFloat(keypoint.Value.R);
                writer.WriteFloat(keypoint.Value.G);
                writer.WriteFloat(keypoint.Value.B);
                writer.WriteInt32(keypoint.Envelope);
            }
        }
        break;
    }
    case PropertyType::PhysicalProperties: {
        // BinaryRobloxFile only appends to RawFlags for values that arrived from a file carrying a
        // RawBuffer, so a column mixing loaded and mounted instances leaves the two vectors
        // different lengths and every index off by however many entries were skipped. The CFrame
        // path guards OrientIds the same way: if the sizes disagree, the preserved bytes are not
        // usable for this column at all.
        const bool preserved = column.RawFlags.size() == count;
        for (size_t index = 0; index < count; ++index) {
            const auto *properties = Get<std::optional<PhysicalProperties>>(normalised[index]);
            if (properties == nullptr)
                return false;
            const bool custom = properties->has_value();
            // PROP.cs:1219-1234 writes a plain bool and exactly five floats -- no other flag bits,
            // and never AcousticAbsorption. A 0.463/0.574 engine reads that leading byte as a bool
            // and then consumes five floats, so a sixth float (or a bit it cannot interpret) puts
            // it four bytes out of alignment and turns the rest of the column into garbage. The
            // extra byte and float are therefore only written back for a value that arrived from a
            // file which itself carried them.
            uint8_t flags = custom ? uint8_t {1} : uint8_t {0};
            if (preserved) {
                flags = column.RawFlags[index];
                flags = custom ? WithFlag(flags, MaterialBitFlags::CustomPhysics)
                    : static_cast<uint8_t>(
                          flags & ~static_cast<uint8_t>(MaterialBitFlags::CustomPhysics));
            }
            writer.WriteByte(flags);
            if (!custom)
                continue;
            const PhysicalProperties &entry = **properties;
            writer.WriteFloat(entry.Density);
            writer.WriteFloat(entry.Friction);
            writer.WriteFloat(entry.Elasticity);
            writer.WriteFloat(entry.FrictionWeight);
            writer.WriteFloat(entry.ElasticityWeight);
            if (preserved && HasFlag(flags, MaterialBitFlags::AcousticAbsorption))
                writer.WriteFloat(entry.AcousticAbsorption);
        }
        break;
    }
    case PropertyType::Color3uint8: {
        std::vector<unsigned char> red, green, blue;
        for (const std::any &value : normalised) {
            Color3uint8 color;
            if (const auto *typed = Get<Color3uint8>(value)) {
                color = *typed;
            } else if (const auto *floats = Get<Color3>(value)) {
                const auto clamp = [](float c) {
                    const float scaled = c * 255.0f + 0.5f;
                    return static_cast<uint8_t>(scaled < 0.0f ? 0.0f
                        : (scaled > 255.0f ? 255.0f : scaled));
                };
                color = Color3uint8(clamp(floats->R), clamp(floats->G), clamp(floats->B));
            } else {
                return false;
            }
            red.push_back(color.R);
            green.push_back(color.G);
            blue.push_back(color.B);
        }
        writer.WriteBytes(red.data(), red.size());
        writer.WriteBytes(green.data(), green.size());
        writer.WriteBytes(blue.data(), blue.size());
        break;
    }
    case PropertyType::UniqueId: {
        std::vector<unsigned char> raw(count * 16);
        for (size_t index = 0; index < count; ++index) {
            const auto *id = Get<UniqueId>(normalised[index]);
            if (id == nullptr)
                return false;
            const uint64_t random = BinaryRobloxFileWriter::RotateInt64(id->Random);
            std::memcpy(raw.data() + index * 16, &random, sizeof(random));
            std::memcpy(raw.data() + index * 16 + 8, &id->Time, sizeof(id->Time));
            std::memcpy(raw.data() + index * 16 + 12, &id->Index, sizeof(id->Index));
        }
        writer.WriteInterleaved(raw, count, 16);
        break;
    }
    case PropertyType::FontFace: {
        for (const std::any &value : normalised) {
            const auto *font = Get<FontFace>(value);
            if (font == nullptr)
                return false;
            writer.WriteString(font->Family);
            const auto weight = static_cast<uint16_t>(font->Weight);
            writer.WriteBytes(&weight, sizeof(weight));
            writer.WriteByte(static_cast<uint8_t>(font->Style));
            writer.WriteString(font->CachedFaceId);
        }
        break;
    }
    case PropertyType::CFrame:
    case PropertyType::Quaternion:
    case PropertyType::OptionalCFrame: {
        if (column.OrientIds.size() != count)
            return false;
        if (column.Type == PropertyType::OptionalCFrame)
            writer.WriteByte(static_cast<uint8_t>(PropertyType::CFrame));

        std::vector<float> x, y, z;
        std::vector<uint8_t> present;
        x.reserve(count);
        y.reserve(count);
        z.reserve(count);

        for (size_t index = 0; index < count; ++index) {
            std::optional<CFrame> frame;
            if (column.Type == PropertyType::OptionalCFrame) {
                const auto *optional = Get<std::optional<CFrame>>(normalised[index]);
                if (optional == nullptr)
                    return false;
                frame = *optional;
                present.push_back(frame.has_value() ? 1 : 0);
                if (!frame.has_value())
                    frame = CFrame();
            } else {
                const auto *value = Get<CFrame>(normalised[index]);
                if (value == nullptr)
                    return false;
                frame = *value;
            }

            const uint8_t orientId = column.OrientIds[index];
            writer.WriteByte(orientId);
            if (orientId == 0) {
                const size_t elements = column.Type == PropertyType::Quaternion ? 4 : 9;
                for (size_t element = 0; element < elements; ++element)
                    writer.WriteFloat(frame->Components[element + 3]);
            }
            x.push_back(frame->Components[0]);
            y.push_back(frame->Components[1]);
            z.push_back(frame->Components[2]);
        }

        writer.WriteFloats(x);
        writer.WriteFloats(y);
        writer.WriteFloats(z);

        if (column.Type == PropertyType::OptionalCFrame) {
            writer.WriteByte(static_cast<uint8_t>(PropertyType::Bool));
            for (uint8_t flag : present)
                writer.WriteByte(flag);
        }
        break;
    }
    case PropertyType::Content: {
        std::vector<int32_t> sourceTypes;
        std::vector<std::string> uris;
        std::vector<int32_t> objectIds;
        sourceTypes.reserve(count);
        for (const std::any &value : normalised) {
            // A Content column can hold a bare string when the source file stored the same
            // property as a String elsewhere; treat it as a URI rather than failing.
            Content content;
            if (const auto *typed = Get<Content>(value)) {
                content = *typed;
            } else if (const auto *text = Get<std::string>(value)) {
                if (!text->empty())
                    content = Content(*text);
            } else {
                return false;
            }
            sourceTypes.push_back(static_cast<int32_t>(content.SourceType));
            if (content.SourceType == ContentSourceType::Uri)
                uris.push_back(content.Uri);
            else if (content.SourceType == ContentSourceType::Object)
                objectIds.push_back(content.ObjectReferent);
        }

        writer.WriteInts(sourceTypes);
        writer.WriteUInt32(static_cast<uint32_t>(uris.size()));
        for (const std::string &uri : uris)
            writer.WriteString(uri);
        writer.WriteInt32(static_cast<int32_t>(objectIds.size()));
        writer.WriteReferents(objectIds);
        writer.WriteInt32(static_cast<int32_t>(column.ExternalContentObjects.size()));
        writer.WriteReferents(column.ExternalContentObjects);
        break;
    }
    default:
        return false;
    }

    return true;
}
