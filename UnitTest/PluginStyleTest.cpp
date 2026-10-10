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
// File: PluginStyleTest.cpp
// Started by: Hattozo
// Started on: 10/10/2026
// Description: Parsing of the style tables plugins declare in their manifest
#include <NoobWarrior/PluginStyle.h>

#include <gtest/gtest.h>

using namespace NoobWarrior;

namespace {
sol::object Evaluate(sol::state &lua, const std::string &source) {
    sol::protected_function_result result = lua.safe_script(source);
    EXPECT_TRUE(result.valid());
    return result.get<sol::object>();
}

std::optional<StyleColor> Color(const std::string &expression) {
    sol::state lua;
    return ParseStyleColor(Evaluate(lua, "return " + expression));
}

struct ParsedStyle {
    bool Parsed;
    DeclaredStyle Style;
    std::vector<std::string> Errors;
};

ParsedStyle Parse(const std::string &source) {
    sol::state lua;
    ParsedStyle parsed {};
    parsed.Parsed = ParseStyleTable(Evaluate(lua, source), parsed.Style, parsed.Errors);
    return parsed;
}
}

TEST(PluginStyle, ParsesRgbArrays) {
    EXPECT_EQ(Color("{ 181, 137, 0 }"), (StyleColor { 181, 137, 0, 255 }));
    EXPECT_EQ(Color("{ 0, 0, 0, 128 }"), (StyleColor { 0, 0, 0, 128 }));
}

TEST(PluginStyle, ParsesHexStrings) {
    EXPECT_EQ(Color("\"#b58900\""), (StyleColor { 0xb5, 0x89, 0x00, 255 }));
    EXPECT_EQ(Color("\"#B5890080\""), (StyleColor { 0xb5, 0x89, 0x00, 0x80 }));
}

TEST(PluginStyle, RejectsMalformedColors) {
    EXPECT_FALSE(Color("{ 256, 0, 0 }"));
    EXPECT_FALSE(Color("{ -1, 0, 0 }"));
    EXPECT_FALSE(Color("{ 0.5, 0, 0 }"));
    EXPECT_FALSE(Color("{ 0, 0 }"));
    EXPECT_FALSE(Color("{ 0, 0, 0, 0, 0 }"));
    EXPECT_FALSE(Color("{ 0, 0, 0, extra = 1 }"));
    EXPECT_FALSE(Color("{ r = 0, g = 0, b = 0 }"));
    EXPECT_FALSE(Color("\"b58900\""));
    EXPECT_FALSE(Color("\"#b5890\""));
    EXPECT_FALSE(Color("\"#gg8900\""));
    EXPECT_FALSE(Color("12"));
}

TEST(PluginStyle, DefaultsToFluentAndBothSchemes) {
    ParsedStyle parsed = Parse("return { colors = { accent = { 1, 2, 3 } } }");
    ASSERT_TRUE(parsed.Parsed);
    EXPECT_TRUE(parsed.Errors.empty());
    EXPECT_EQ(parsed.Style.Base, "fluent");
    EXPECT_EQ(parsed.Style.SharedColors.at("accent"), (StyleColor { 1, 2, 3, 255 }));
    EXPECT_TRUE(parsed.Style.SupportsDark());
    EXPECT_TRUE(parsed.Style.SupportsLight());
}

TEST(PluginStyle, ReadsDarkAndLightVariants) {
    ParsedStyle parsed = Parse(R"(return {
        version = 1,
        base = "darcula",
        colors = { accent = "#b58900" },
        dark = { colors = { shell = { 0, 43, 54 } } },
        light = { colors = { shell = { 253, 246, 227 } } },
    })");
    ASSERT_TRUE(parsed.Parsed);
    EXPECT_TRUE(parsed.Errors.empty());
    EXPECT_EQ(parsed.Style.Base, "darcula");
    ASSERT_TRUE(parsed.Style.DarkColors);
    ASSERT_TRUE(parsed.Style.LightColors);
    EXPECT_EQ(parsed.Style.DarkColors->at("shell"), (StyleColor { 0, 43, 54, 255 }));
    EXPECT_EQ(parsed.Style.LightColors->at("shell"), (StyleColor { 253, 246, 227, 255 }));
}

TEST(PluginStyle, SingleVariantStyleSupportsOnlyThatScheme) {
    ParsedStyle parsed = Parse("return { dark = { colors = {} } }");
    ASSERT_TRUE(parsed.Parsed);
    EXPECT_TRUE(parsed.Style.SupportsDark());
    EXPECT_FALSE(parsed.Style.SupportsLight());
}

TEST(PluginStyle, SkipsBadColorsButKeepsTheStyle) {
    ParsedStyle parsed = Parse("return { colors = { accent = { 1, 2, 3 }, shell = { 999, 0, 0 } }, spare = true }");
    ASSERT_TRUE(parsed.Parsed);
    EXPECT_TRUE(parsed.Style.SharedColors.contains("accent"));
    EXPECT_FALSE(parsed.Style.SharedColors.contains("shell"));
    EXPECT_EQ(parsed.Errors.size(), 2u);
}

TEST(PluginStyle, RefusesANewerFormatVersion) {
    ParsedStyle parsed = Parse("return { version = 2 }");
    EXPECT_FALSE(parsed.Parsed);
    EXPECT_EQ(parsed.Errors.size(), 1u);
}

TEST(PluginStyle, RefusesNonTablesAndBadFields) {
    EXPECT_FALSE(Parse("return 5").Parsed);
    EXPECT_FALSE(Parse("return { base = 3 }").Parsed);
    EXPECT_FALSE(Parse("return { version = 1.5 }").Parsed);
    EXPECT_FALSE(Parse("return { dark = \"#000000\" }").Parsed);
}

TEST(PluginStyle, ReadsWebStylesheets) {
    ParsedStyle parsed = Parse(R"(return { web = { emu = "web/emu.css", master = "web/master.css" } })");
    ASSERT_TRUE(parsed.Parsed);
    EXPECT_TRUE(parsed.Errors.empty());
    EXPECT_EQ(parsed.Style.WebStylesheets.at("emu"), "web/emu.css");
    EXPECT_EQ(parsed.Style.WebStylesheets.at("master"), "web/master.css");
}

TEST(PluginStyle, SkipsUnknownAndMalformedWebEntries) {
    ParsedStyle parsed = Parse(R"(return { web = { emu = 5, master = "web/master.css", docs = "web/docs.css" } })");
    ASSERT_TRUE(parsed.Parsed);
    EXPECT_FALSE(parsed.Style.WebStylesheets.contains("emu"));
    EXPECT_FALSE(parsed.Style.WebStylesheets.contains("docs"));
    EXPECT_TRUE(parsed.Style.WebStylesheets.contains("master"));
    EXPECT_EQ(parsed.Errors.size(), 2u);
}

TEST(PluginStyle, RefusesAWebFieldThatIsNotATable) {
    EXPECT_FALSE(Parse(R"(return { web = "web/emu.css" })").Parsed);
}

TEST(PluginStyle, QualifiedIdJoinsPluginAndStyle) {
    DeclaredStyle style;
    style.OwnerIdentifier = "solarized";
    style.Id = "classic";
    EXPECT_EQ(style.GetQualifiedId(), "solarized/classic");
}
