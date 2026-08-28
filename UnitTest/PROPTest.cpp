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
// File: PROPTest.cpp
// Started by: Hattozo
// Started on: 8/18/2025
// Description: PROP value-column codec, re-encoded byte-for-byte against installed places.
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/Chunks/PROP.h>
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryRobloxFile.h>
#include <NoobWarrior/Roblox/FileFormat/RobloxFile.h>

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace NoobWarrior::Roblox;
using namespace NoobWarrior::Roblox::DataTypes;

namespace {
bool RoundTrip(const PROP::ValueColumn &column, PROP::ValueColumn &decoded) {
    BinaryRobloxFileWriter writer;
    if (!PROP::Write(writer, column))
        return false;
    BinaryRobloxFileReader reader(writer.Data());
    if (!PROP::Read(reader, column.Type, column.Values.size(), decoded))
        return false;
    return !reader.Failed() && reader.Remaining() == 0;
}

template<typename T>
void ExpectValues(const PROP::ValueColumn &decoded, const std::vector<T> &expected) {
    ASSERT_EQ(expected.size(), decoded.Values.size());
    for (size_t index = 0; index < expected.size(); ++index) {
        const T *value = std::any_cast<T>(&decoded.Values[index]);
        ASSERT_NE(nullptr, value) << "value " << index << " has the wrong type";
        EXPECT_EQ(expected[index], *value) << "value " << index;
    }
}

PROP::ValueColumn MakeColumn(PropertyType type, std::vector<std::any> values) {
    PROP::ValueColumn column;
    column.Type = type;
    column.Values = std::move(values);
    return column;
}
} // namespace

TEST(PROP, ScalarColumnsRoundTrip) {
    PROP::ValueColumn decoded;

    auto strings = MakeColumn(PropertyType::String,
                              {std::string("alpha"), std::string(""), std::string("\x01\x02", 2)});
    ASSERT_TRUE(RoundTrip(strings, decoded));
    ExpectValues<std::string>(decoded, {"alpha", "", std::string("\x01\x02", 2)});

    auto bools = MakeColumn(PropertyType::Bool, {true, false, true});
    ASSERT_TRUE(RoundTrip(bools, decoded));
    ExpectValues<bool>(decoded, {true, false, true});

    auto ints = MakeColumn(PropertyType::Int, {int32_t {0}, int32_t {-5}, int32_t {123456}});
    ASSERT_TRUE(RoundTrip(ints, decoded));
    ExpectValues<int32_t>(decoded, {0, -5, 123456});

    auto longs = MakeColumn(PropertyType::Int64, {int64_t {-1}, int64_t {8589934592LL}});
    ASSERT_TRUE(RoundTrip(longs, decoded));
    ExpectValues<int64_t>(decoded, {-1, 8589934592LL});

    auto doubles = MakeColumn(PropertyType::Double, {1.5, -0.25});
    ASSERT_TRUE(RoundTrip(doubles, decoded));
    ExpectValues<double>(decoded, {1.5, -0.25});

    auto enums = MakeColumn(PropertyType::Enum, {uint32_t {0}, uint32_t {7}});
    ASSERT_TRUE(RoundTrip(enums, decoded));
    ExpectValues<uint32_t>(decoded, {0, 7});

    auto refs = MakeColumn(PropertyType::Ref, {int32_t {-1}, int32_t {4}, int32_t {3}});
    ASSERT_TRUE(RoundTrip(refs, decoded));
    ExpectValues<int32_t>(decoded, {-1, 4, 3});
}

TEST(PROP, CompositeColumnsRoundTrip) {
    PROP::ValueColumn decoded;

    auto udims = MakeColumn(PropertyType::UDim, {UDim(0.5f, -3), UDim(0.0f, 12)});
    ASSERT_TRUE(RoundTrip(udims, decoded));
    ExpectValues<UDim>(decoded, {UDim(0.5f, -3), UDim(0.0f, 12)});

    auto udim2s = MakeColumn(PropertyType::UDim2, {UDim2(0.25f, 4, 0.75f, -8)});
    ASSERT_TRUE(RoundTrip(udim2s, decoded));
    ExpectValues<UDim2>(decoded, {UDim2(0.25f, 4, 0.75f, -8)});

    auto rects = MakeColumn(PropertyType::Rect, {Rect({1, 2}, {3, 4})});
    ASSERT_TRUE(RoundTrip(rects, decoded));
    ExpectValues<Rect>(decoded, {Rect({1, 2}, {3, 4})});

    auto rays = MakeColumn(PropertyType::Ray, {Ray({1, 2, 3}, {4, 5, 6})});
    ASSERT_TRUE(RoundTrip(rays, decoded));
    ExpectValues<Ray>(decoded, {Ray({1, 2, 3}, {4, 5, 6})});

    auto faces = MakeColumn(PropertyType::Faces, {Faces(0x2A), Faces(0)});
    ASSERT_TRUE(RoundTrip(faces, decoded));
    ExpectValues<Faces>(decoded, {Faces(0x2A), Faces(0)});

    auto axes = MakeColumn(PropertyType::Axes, {Axes(0x05)});
    ASSERT_TRUE(RoundTrip(axes, decoded));
    ExpectValues<Axes>(decoded, {Axes(0x05)});

    auto colors = MakeColumn(PropertyType::Color3, {Color3(0.1f, 0.2f, 0.3f)});
    ASSERT_TRUE(RoundTrip(colors, decoded));
    ExpectValues<Color3>(decoded, {Color3(0.1f, 0.2f, 0.3f)});

    auto colors8 = MakeColumn(PropertyType::Color3uint8, {Color3uint8(1, 2, 3), Color3uint8(255, 0, 128)});
    ASSERT_TRUE(RoundTrip(colors8, decoded));
    ExpectValues<Color3uint8>(decoded, {Color3uint8(1, 2, 3), Color3uint8(255, 0, 128)});

    auto vectors = MakeColumn(PropertyType::Vector3, {Vector3(1, -2, 3.5f)});
    ASSERT_TRUE(RoundTrip(vectors, decoded));
    ExpectValues<Vector3>(decoded, {Vector3(1, -2, 3.5f)});

    auto shorts = MakeColumn(PropertyType::Vector3int16, {Vector3int16(-1, 2, -3)});
    ASSERT_TRUE(RoundTrip(shorts, decoded));
    ExpectValues<Vector3int16>(decoded, {Vector3int16(-1, 2, -3)});

    auto ranges = MakeColumn(PropertyType::NumberRange, {NumberRange(1.0f, 2.0f)});
    ASSERT_TRUE(RoundTrip(ranges, decoded));
    ExpectValues<NumberRange>(decoded, {NumberRange(1.0f, 2.0f)});

    auto caps = MakeColumn(PropertyType::SecurityCapabilities,
                           {SecurityCapabilities(0), SecurityCapabilities(0xDEADBEEFCAFEULL)});
    ASSERT_TRUE(RoundTrip(caps, decoded));
    ExpectValues<SecurityCapabilities>(decoded,
                                       {SecurityCapabilities(0), SecurityCapabilities(0xDEADBEEFCAFEULL)});

    auto ids = MakeColumn(PropertyType::UniqueId,
                          {UniqueId(-1234567890123LL, 0x11223344u, 0x55667788u), UniqueId(0, 0, 0)});
    ASSERT_TRUE(RoundTrip(ids, decoded));
    ExpectValues<UniqueId>(decoded,
                           {UniqueId(-1234567890123LL, 0x11223344u, 0x55667788u), UniqueId(0, 0, 0)});

    auto fonts = MakeColumn(PropertyType::FontFace,
                            {FontFace("rbxasset://fonts/families/SourceSansPro.json",
                                      FontWeight::Bold, FontStyle::Italic, "cached")});
    ASSERT_TRUE(RoundTrip(fonts, decoded));
    ExpectValues<FontFace>(decoded,
                           {FontFace("rbxasset://fonts/families/SourceSansPro.json",
                                     FontWeight::Bold, FontStyle::Italic, "cached")});

    auto protectedStrings = MakeColumn(PropertyType::ProtectedString,
                                       {ProtectedString(std::string("print('hi')"))});
    ASSERT_TRUE(RoundTrip(protectedStrings, decoded));
    ExpectValues<ProtectedString>(decoded, {ProtectedString(std::string("print('hi')"))});
}

TEST(PROP, SequenceAndOptionalColumnsRoundTrip) {
    PROP::ValueColumn decoded;

    NumberSequence numbers({{0.0f, 1.0f, 0.0f}, {1.0f, 0.5f, 0.25f}});
    auto numberColumn = MakeColumn(PropertyType::NumberSequence, {numbers});
    ASSERT_TRUE(RoundTrip(numberColumn, decoded));
    ExpectValues<NumberSequence>(decoded, {numbers});
    
    ColorSequence colors({{0.0f, Color3(1, 0, 0), 0}, {1.0f, Color3(0, 0, 1), 0}});
    auto colorColumn = MakeColumn(PropertyType::ColorSequence, {colors});
    ASSERT_TRUE(RoundTrip(colorColumn, decoded));
    ExpectValues<ColorSequence>(decoded, {colors});

    PhysicalProperties custom(0.7f, 0.3f, 0.5f, 1.0f, 2.0f);
    custom.Flags = 0x01;
    auto physical = MakeColumn(PropertyType::PhysicalProperties,
                               {std::optional<PhysicalProperties>(custom),
                                std::optional<PhysicalProperties>()});
    ASSERT_TRUE(RoundTrip(physical, decoded));
    ExpectValues<std::optional<PhysicalProperties>>(
        decoded, {std::optional<PhysicalProperties>(custom), std::optional<PhysicalProperties>()});

    // The AcousticAbsorption bit is what upstream's writer drops.
    PhysicalProperties acoustic(0.7f, 0.3f, 0.5f, 1.0f, 2.0f);
    acoustic.Flags = 0x03;
    acoustic.AcousticAbsorption = 0.42f;
    auto acousticColumn = MakeColumn(PropertyType::PhysicalProperties,
                                     {std::optional<PhysicalProperties>(acoustic)});
    ASSERT_TRUE(RoundTrip(acousticColumn, decoded));
    ExpectValues<std::optional<PhysicalProperties>>(
        decoded, {std::optional<PhysicalProperties>(acoustic)});
}

TEST(PROP, CFrameColumnsRoundTrip) {
    PROP::ValueColumn decoded;

    PROP::ValueColumn packed;
    packed.Type = PropertyType::CFrame;
    packed.Values = {CFrame(Vector3(1, 2, 3))};
    packed.OrientIds = {static_cast<uint8_t>(CFrame(Vector3(1, 2, 3)).GetOrientId() + 1)};
    ASSERT_TRUE(RoundTrip(packed, decoded));
    ASSERT_EQ(1u, decoded.Values.size());
    EXPECT_EQ(Vector3(1, 2, 3), std::any_cast<CFrame>(decoded.Values[0]).Position());
    EXPECT_EQ(packed.OrientIds, decoded.OrientIds);

    // A non-axis-aligned rotation must serialize the raw matrix.
    std::array<float, 12> tilted {{4, 5, 6, 0.7071f, -0.7071f, 0, 0.7071f, 0.7071f, 0, 0, 0, 1}};
    PROP::ValueColumn raw;
    raw.Type = PropertyType::CFrame;
    raw.Values = {CFrame(tilted)};
    raw.OrientIds = {0};
    ASSERT_TRUE(RoundTrip(raw, decoded));
    EXPECT_EQ(CFrame(tilted), std::any_cast<CFrame>(decoded.Values[0]));

    PROP::ValueColumn optional;
    optional.Type = PropertyType::OptionalCFrame;
    optional.Values = {std::optional<CFrame>(CFrame(Vector3(7, 8, 9))), std::optional<CFrame>()};
    optional.OrientIds = {static_cast<uint8_t>(CFrame().GetOrientId() + 1),
                          static_cast<uint8_t>(CFrame().GetOrientId() + 1)};
    ASSERT_TRUE(RoundTrip(optional, decoded));
    ASSERT_EQ(2u, decoded.Values.size());
    EXPECT_TRUE(std::any_cast<std::optional<CFrame>>(decoded.Values[0]).has_value());
    EXPECT_FALSE(std::any_cast<std::optional<CFrame>>(decoded.Values[1]).has_value());
}

TEST(PROP, ContentColumnRoundTrips) {
    PROP::ValueColumn column;
    column.Type = PropertyType::Content;
    Content uri("rbxassetid://12345");
    Content object;
    object.SourceType = ContentSourceType::Object;
    object.ObjectReferent = 9;
    column.Values = {uri, object, Content()};
    column.ExternalContentObjects = {3, 4};

    PROP::ValueColumn decoded;
    ASSERT_TRUE(RoundTrip(column, decoded));
    ASSERT_EQ(3u, decoded.Values.size());
    EXPECT_EQ("rbxassetid://12345", std::any_cast<Content>(decoded.Values[0]).Uri);
    EXPECT_EQ(9, std::any_cast<Content>(decoded.Values[1]).ObjectReferent);
    EXPECT_EQ(ContentSourceType::None, std::any_cast<Content>(decoded.Values[2]).SourceType);
    EXPECT_EQ(column.ExternalContentObjects, decoded.ExternalContentObjects);
}

// The strongest available check: decode every PROP column of every installed place and require the
// re-encoded bytes to match the original file exactly.
TEST(PROP, ReencodesInstalledPlacesByteForByte) {
    const char *root = std::getenv("NOOBWARRIOR_ENGINES_DIR");
    if (root == nullptr)
        GTEST_SKIP() << "set NOOBWARRIOR_ENGINES_DIR to cross-check against installed places";

    int columns = 0;
    int places = 0;
    int skippedUnsupported = 0;
    std::map<int, int> mismatchesByType;
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

        for (const auto &chunk : document.Chunks) {
            if (!chunk.Is("PROP"))
                continue;

            BinaryRobloxFileReader header(chunk.Data);
            const uint32_t classId = header.ReadUInt32();
            header.ReadString();
            const uint8_t rawType = header.ReadByte();
            if (header.Failed())
                continue;

            const auto *classInfo = document.FindClass(static_cast<int32_t>(classId));
            if (classInfo == nullptr)
                continue;
            const size_t count = classInfo->ObjectIds.size();
            const auto type = static_cast<PropertyType>(rawType);
            if (!PROP::IsSupported(type)) {
                ++skippedUnsupported;
                continue;
            }

            const size_t valueStart = header.Position();
            BinaryRobloxFileReader reader(chunk.Data);
            reader.Seek(valueStart);
            PROP::ValueColumn decodedColumn;
            if (!PROP::Read(reader, type, count, decodedColumn)) {
                ++mismatchesByType[rawType];
                continue;
            }

            BinaryRobloxFileWriter writer;
            if (!PROP::Write(writer, decodedColumn)) {
                ++mismatchesByType[rawType];
                continue;
            }

            const std::vector<unsigned char> original(
                chunk.Data.begin() + static_cast<std::ptrdiff_t>(valueStart), chunk.Data.end());
            if (original != writer.Data())
                ++mismatchesByType[rawType];
            ++columns;
        }
    }

    for (const auto &[type, count] : mismatchesByType) {
        ADD_FAILURE() << "type 0x" << std::hex << type << std::dec
                      << " failed to re-encode in " << count << " columns";
    }
    std::cout << "re-encoded " << columns << " PROP columns across " << places
              << " places (" << skippedUnsupported << " unsupported types skipped)\n";
    EXPECT_GT(columns, 0);
}

TEST(PROPCodec, CoercesMixedColourRepresentationsToTheColumnType) {
    // A place stores BasePart.Color one way and a mounted plugin model the other; BuildTables
    // merges them into a single chunk, so the writer has to normalise.
    PROP::ValueColumn asColor3;
    asColor3.Type = PropertyType::Color3;
    asColor3.Values = {Color3(1.0f, 0.0f, 0.5f), Color3uint8(0, 255, 128)};
    BinaryRobloxFileWriter first;
    ASSERT_TRUE(PROP::Write(first, asColor3)) << "Color3uint8 in a Color3 column must coerce";

    PROP::ValueColumn decoded;
    BinaryRobloxFileReader reader(first.Data());
    ASSERT_TRUE(PROP::Read(reader, PropertyType::Color3, 2, decoded));
    EXPECT_EQ(Color3(1.0f, 0.0f, 0.5f), *std::any_cast<Color3>(&decoded.Values[0]));

    PROP::ValueColumn asPacked;
    asPacked.Type = PropertyType::Color3uint8;
    asPacked.Values = {Color3uint8(10, 20, 30), Color3(1.0f, 0.0f, 0.0f)};
    BinaryRobloxFileWriter second;
    ASSERT_TRUE(PROP::Write(second, asPacked)) << "Color3 in a Color3uint8 column must coerce";

    BinaryRobloxFileReader back(second.Data());
    ASSERT_TRUE(PROP::Read(back, PropertyType::Color3uint8, 2, decoded));
    EXPECT_EQ(Color3uint8(10, 20, 30), *std::any_cast<Color3uint8>(&decoded.Values[0]));
    EXPECT_EQ(Color3uint8(255, 0, 0), *std::any_cast<Color3uint8>(&decoded.Values[1]));
}

// Every representation mismatch that has surfaced from mounting plugin content into a place.
// Writing the wrong one yields a file Studio refuses to open, so the column type always wins.
TEST(PROPCodec, NormalisesEveryValueToTheColumnType) {
    struct Case { const char *what; PropertyType type; std::vector<std::any> values; };
    const std::vector<Case> cases {
        {"Content in a String column", PropertyType::String,
         {std::string("a"), Content("rbxassetid://1"), ProtectedString(std::string("b"))}},
        {"String in a Content column", PropertyType::Content,
         {Content("rbxassetid://1"), std::string("rbxassetid://2")}},
        {"String in a ProtectedString column", PropertyType::ProtectedString,
         {ProtectedString(std::string("x")), std::string("y")}},
        {"Color3uint8 in a Color3 column", PropertyType::Color3,
         {Color3(1, 0, 0), Color3uint8(0, 255, 0)}},
        {"Color3 in a Color3uint8 column", PropertyType::Color3uint8,
         {Color3uint8(1, 2, 3), Color3(1, 0, 0)}},
        {"Int64 in an Int column", PropertyType::Int, {int32_t {1}, int64_t {2}}},
        {"Int32 in an Int64 column", PropertyType::Int64, {int64_t {1}, int32_t {2}}},
        {"Double in a Float column", PropertyType::Float, {1.0f, 2.0}},
        {"Float in a Double column", PropertyType::Double, {1.0, 2.0f}},
        {"Int in an Enum column", PropertyType::Enum, {uint32_t {1}, int32_t {2}}},
        {"Vector3int16 in a Vector3 column", PropertyType::Vector3,
         {Vector3(1, 2, 3), Vector3int16(4, 5, 6)}},
    };

    for (const Case &entry : cases) {
        PROP::ValueColumn column;
        column.Type = entry.type;
        column.Values = entry.values;
        BinaryRobloxFileWriter writer;
        ASSERT_TRUE(PROP::Write(writer, column)) << entry.what;

        // The bytes must decode as the declared type, which is what Studio validates.
        PROP::ValueColumn decoded;
        BinaryRobloxFileReader reader(writer.Data());
        ASSERT_TRUE(PROP::Read(reader, entry.type, entry.values.size(), decoded)) << entry.what;
        EXPECT_EQ(0u, reader.Remaining()) << entry.what;
        EXPECT_EQ(entry.values.size(), decoded.Values.size()) << entry.what;
    }
}

// The API dump is the authority on a property's declared type. These four are exactly the
// mismatches that produced places Studio refused to open.
