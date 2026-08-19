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
// File: PluginPlanFidelityTest.cpp
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Checks that the plan encoder and decoder agree for every XML property token.
#include <NoobWarrior/PluginDataModel.h>
#include <NoobWarrior/PluginTreeMaterializer.h>
#include <NoobWarrior/Roblox/FileFormat/XmlFormat/XmlPropertyTokens.h>

#include <gtest/gtest.h>

#include <pugixml.hpp>

#include <string>
#include <vector>

using NoobWarrior::ConvertPlanValue;
using NoobWarrior::XmlPropertyValue;
using NoobWarrior::Roblox::Property;
using NoobWarrior::Roblox::PropertyType;

namespace {
// Mounting a plugin's .rbxmx does not hand the parsed tree to the place. Each property is encoded
// into a tagged plan value by XmlPropertyValue and decoded back by ConvertPlanValue, so those two
// have to agree. Every disagreement found so far was a real bug that reached a user's place:
// <Content> collapsed onto ContentId, <int64> landing as PropertyType::Int and truncating asset
// ids, <double> as Float, NumberRange read from <Min>/<Max> children Roblox has never written,
// and Font losing its weight and style.
//
// The oracle is the token registry -- the path an .rbxlx place takes when it is loaded directly.
// If the plan round trip disagrees with it, mounted content differs from the same content opened
// in Studio.
struct TokenCase {
    const char *Token;          // the XML element name a real file uses
    const char *Xml;            // one property exactly as Roblox writes it
    const char *PropertyName;
};

// One case per token the plan is expected to carry. A token missing from this list is either
// genuinely absent from the plan (assert that below) or an untested gap.
const std::vector<TokenCase> &Cases() {
    static const std::vector<TokenCase> cases = {
        {"string", R"(<string name="P">hello</string>)", "P"},
        {"ProtectedString", R"(<ProtectedString name="Source">print(1)</ProtectedString>)", "Source"},
        {"bool", R"(<bool name="P">true</bool>)", "P"},
        {"int", R"(<int name="P">-42</int>)", "P"},
        {"int64", R"(<int64 name="P">9007199254740993</int64>)", "P"},
        {"float", R"(<float name="P">0.25</float>)", "P"},
        {"double", R"(<double name="P">0.1</double>)", "P"},
        {"token", R"(<token name="P">6</token>)", "P"},
        {"BrickColor", R"(<BrickColor name="P">194</BrickColor>)", "P"},
        {"Color3", R"(<Color3 name="P"><R>0.25</R><G>0.5</G><B>0.75</B></Color3>)", "P"},
        {"Color3uint8", R"(<Color3uint8 name="Color3uint8">4289479591</Color3uint8>)", "Color3uint8"},
        {"Vector2", R"(<Vector2 name="P"><X>1.5</X><Y>2.5</Y></Vector2>)", "P"},
        {"Vector3", R"(<Vector3 name="P"><X>1</X><Y>2</Y><Z>3</Z></Vector3>)", "P"},
        {"Vector3int16", R"(<Vector3int16 name="P"><X>1</X><Y>2</Y><Z>3</Z></Vector3int16>)", "P"},
        {"UDim", R"(<UDim name="P"><S>0.5</S><O>10</O></UDim>)", "P"},
        {"UDim2", R"(<UDim2 name="P"><XS>0.5</XS><XO>10</XO><YS>0.25</YS><YO>20</YO></UDim2>)", "P"},
        {"Rect2D", R"(<Rect2D name="P"><min><X>1</X><Y>2</Y></min><max><X>3</X><Y>4</Y></max></Rect2D>)", "P"},
        {"NumberRange", R"(<NumberRange name="P">2 5 </NumberRange>)", "P"},
        {"Axes", R"(<Axes name="P"><axes>7</axes></Axes>)", "P"},
        {"Faces", R"(<Faces name="P"><faces>63</faces></Faces>)", "P"},
        {"NumberSequence", R"(<NumberSequence name="P">0 1 0 1 0.5 0 </NumberSequence>)", "P"},
        {"ColorSequence", R"(<ColorSequence name="P">0 1 0 0 0 1 0 0 1 0 </ColorSequence>)", "P"},
        {"BinaryString", R"(<BinaryString name="P">SGVsbG8=</BinaryString>)", "P"},
        {"Content", R"(<Content name="P"><url>rbxassetid://123</url></Content>)", "P"},
        {"CoordinateFrame", R"(<CoordinateFrame name="P"><X>1</X><Y>2</Y><Z>3</Z><R00>1</R00><R01>0</R01><R02>0</R02><R10>0</R10><R11>1</R11><R12>0</R12><R20>0</R20><R21>0</R21><R22>1</R22></CoordinateFrame>)", "P"},
        {"Font", R"(<Font name="FontFace"><Family><url>rbxasset://fonts/families/SourceSansPro.json</url></Family><Weight>700</Weight><Style>Italic</Style></Font>)", "FontFace"},
    };
    return cases;
}

// Parses one property element and fills the Property the way XmlRobloxFile::LoadItem does, so the
// producer sees exactly what it sees in the real load path.
Property Parse(pugi::xml_document &document, const TokenCase &testCase) {
    EXPECT_TRUE(document.load_string(testCase.Xml)) << testCase.Token;
    const pugi::xml_node node = document.first_child();
    Property property;
    property.Name = testCase.PropertyName;
    property.XmlToken = node.name();
    std::ostringstream raw;
    node.print(raw, "", pugi::format_raw);
    const std::string text = raw.str();
    property.RawBuffer.assign(text.begin(), text.end());
    return property;
}
} // namespace

// The type the plan carries back must match the type the token registry produces for the very same
// element. A mismatch means mounted content lands with a different PropertyType than the identical
// content loaded straight from an .rbxlx -- which is how a Color3uint8 ended up in a Color column.
TEST(PluginPlanFidelity, EveryTokenKeepsItsPropertyType) {
    for (const TokenCase &testCase : Cases()) {
        pugi::xml_document document;
        Property property = Parse(document, testCase);

        const auto *token = NoobWarrior::Roblox::XmlFormat::FindToken(property.XmlToken);
        ASSERT_NE(nullptr, token) << testCase.Token;
        Property direct;
        direct.Name = property.Name;
        direct.XmlToken = property.XmlToken;
        ASSERT_TRUE(token->ReadProperty(direct, document.first_child())) << testCase.Token;

        std::string outputName;
        std::string unsupported;
        const std::optional<nlohmann::json> planned =
            XmlPropertyValue(property, "NS", outputName, &unsupported);
        ASSERT_TRUE(planned.has_value())
            << testCase.Token << " has no plan encoding (unsupported=" << unsupported << ")";

        PropertyType replayed = PropertyType::Unknown;
        std::any value;
        ASSERT_TRUE(ConvertPlanValue(*planned, replayed, value))
            << testCase.Token << " encodes to a plan value the materializer cannot read";

        EXPECT_EQ(direct.Type, replayed)
            << testCase.Token << ": loading it directly gives PropertyType "
            << static_cast<int>(direct.Type) << " but the plan replays it as "
            << static_cast<int>(replayed);
    }
}

// The plan must never rename a property. Color3uint8/size/shape are what a file stores; Color/Size/
// Shape are scripting aliases that appear in no file, and emitting one adds a column to the class
// that every other instance then has to carry.
TEST(PluginPlanFidelity, PlanNeverRenamesASerializedProperty) {
    for (const TokenCase &testCase : Cases()) {
        pugi::xml_document document;
        Property property = Parse(document, testCase);
        std::string outputName;
        XmlPropertyValue(property, "NS", outputName, nullptr);
        EXPECT_EQ(property.Name, outputName)
            << testCase.Token << ": the plan renamed " << property.Name << " to " << outputName;
    }
}

// A value the plan cannot represent has to be reported, not dropped in silence -- a silently
// missing property is indistinguishable from one the file never had.
TEST(PluginPlanFidelity, UnsupportedTokensAreReported) {
    pugi::xml_document document;
    ASSERT_TRUE(document.load_string(R"(<SecurityCapabilities name="P">5</SecurityCapabilities>)"));
    Property property;
    property.Name = "P";
    property.XmlToken = "SecurityCapabilities";
    std::ostringstream raw;
    document.first_child().print(raw, "", pugi::format_raw);
    const std::string text = raw.str();
    property.RawBuffer.assign(text.begin(), text.end());

    std::string outputName;
    std::string unsupported;
    const std::optional<nlohmann::json> planned =
        XmlPropertyValue(property, "NS", outputName, &unsupported);
    if (!planned.has_value())
        EXPECT_FALSE(unsupported.empty()) << "dropped without naming the token";
}
