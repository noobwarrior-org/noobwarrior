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
// File: XmlPropertyTokensTest.cpp
// Started by: Hattozo
// Started on: 8/18/2025
// Description: XML property tokens, round-tripped over the repository .rbxmx assets.
#include <NoobWarrior/Roblox/FileFormat/XmlFormat/XmlPropertyTokens.h>

#include <ranges>
#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>

using namespace NoobWarrior::Roblox;
using namespace NoobWarrior::Roblox::Tokens;
using namespace NoobWarrior::Roblox::XmlFormat;

namespace {
std::string Serialize(const pugi::xml_node &node) {
    std::ostringstream stream;
    node.print(stream, "", pugi::format_raw);
    return stream.str();
}

// Writing a value the token just read must be idempotent: the first write normalises whatever
// spelling the file used, and every later write must reproduce it exactly.
void ExpectStable(const IXmlPropertyToken &token, const pugi::xml_node &source,
                  const std::string &where, int &checked) {
    Property first;
    if (!token.ReadProperty(first, source))
        return;

    pugi::xml_document a;
    pugi::xml_node nodeA = a.append_child(std::string(token.XmlPropertyToken()).c_str());
    token.WriteProperty(first, nodeA);

    Property second;
    ASSERT_TRUE(token.ReadProperty(second, nodeA))
        << where << ": could not re-read what the token wrote";
    pugi::xml_document b;
    pugi::xml_node nodeB = b.append_child(std::string(token.XmlPropertyToken()).c_str());
    token.WriteProperty(second, nodeB);

    EXPECT_EQ(Serialize(nodeA), Serialize(nodeB)) << where << " is not stable under re-encoding";
    EXPECT_EQ(first.Type, second.Type) << where;
    ++checked;
}
} // namespace

TEST(XmlPropertyTokens, RegistryCoversEveryTokenName) {
    for (const char *name : {"Axes", "BinaryString", "bool", "BrickColor", "CoordinateFrame",
                             "CFrame", "Color3", "Color3uint8", "ColorSequence", "Content",
                             "ContentId", "double", "token", "Faces", "float", "Font", "int",
                             "int64", "NumberRange", "NumberSequence", "OptionalCoordinateFrame",
                             "PhysicalProperties", "ProtectedString", "Ray", "Rect2D", "Ref",
                             "SecurityCapabilities", "SharedString", "string", "UDim", "UDim2",
                             "UniqueId", "Vector2", "Vector3", "Vector3int16"}) {
        EXPECT_NE(nullptr, FindToken(name)) << "no token registered for <" << name << ">";
    }
    EXPECT_EQ(nullptr, FindToken("NotARealToken"));
}

TEST(XmlPropertyTokens, RoundTripRepositoryModels) {
    const char *root = std::getenv("NOOBWARRIOR_REPO_DIR");
    if (root == nullptr)
        GTEST_SKIP() << "set NOOBWARRIOR_REPO_DIR to exercise the repository's .rbxmx assets";

    int checked = 0, files = 0, unknown = 0;
    std::set<std::string> seen;
    std::map<std::string, int> unknownTokens;
    std::error_code code;

    for (auto it = std::filesystem::recursive_directory_iterator(root, code);
         !code && it != std::filesystem::recursive_directory_iterator(); it.increment(code)) {
        const auto ext = it->path().extension();
        if (ext != ".rbxmx" && ext != ".rbxlx")
            continue;
        pugi::xml_document document;
        if (!document.load_file(it->path().string().c_str()))
            continue;
        ++files;

        for (pugi::xml_node properties : document.select_nodes("//Properties") |
                 std::views::transform([](const pugi::xpath_node &n) { return n.node(); })) {
            for (pugi::xml_node value : properties.children()) {
                const std::string name = value.name();
                const IXmlPropertyToken *token = FindToken(name);
                if (token == nullptr) {
                    ++unknownTokens[name];
                    ++unknown;
                    continue;
                }
                seen.insert(name);
                ExpectStable(*token, value,
                             it->path().filename().string() + " <" + name + ">", checked);
            }
        }
    }

    for (const auto &[name, count] : unknownTokens)
        ADD_FAILURE() << "no token for <" << name << "> (" << count << " occurrences)";
    std::cout << "round-tripped " << checked << " property nodes across " << files
              << " files, covering " << seen.size() << " distinct tokens, "
              << unknown << " unknown\n";
    EXPECT_GT(checked, 0);
}
