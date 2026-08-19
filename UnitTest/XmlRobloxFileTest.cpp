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
// File: XmlRobloxFileTest.cpp
// Started by: Hattozo
// Started on: 8/18/2025
// Description: XML load -> save -> load tree equality over the repository .rbxmx assets.
#include <NoobWarrior/Roblox/FileFormat/XmlFormat/XmlRobloxFile.h>
#include <algorithm>

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

using NoobWarrior::Roblox::Instance;
using NoobWarrior::Roblox::XmlFormat::XmlRobloxFile;

namespace {
void CompareTree(Instance &a, Instance &b, const std::string &where) {
    EXPECT_EQ(a.ClassName, b.ClassName) << where;
    EXPECT_EQ(a.Name, b.Name) << where;
    EXPECT_EQ(a.GetProperties().size(), b.GetProperties().size())
        << where << " " << a.ClassName << "." << a.Name;
    for (const auto &[name, property] : a.GetProperties()) {
        const auto found = b.GetProperties().find(name);
        ASSERT_NE(b.GetProperties().end(), found) << where << " lost " << a.ClassName << "." << name;
        EXPECT_EQ(property.XmlToken, found->second.XmlToken) << where << " " << name;
        EXPECT_EQ(property.Type, found->second.Type) << where << " " << name;
    }

    std::vector<Instance *> left = a.GetChildren();
    std::vector<Instance *> right = b.GetChildren();
    ASSERT_EQ(left.size(), right.size()) << where << " child count for " << a.Name;
    auto byReferent = [](const Instance *x, const Instance *y) { return x->Referent < y->Referent; };
    std::sort(left.begin(), left.end(), byReferent);
    std::sort(right.begin(), right.end(), byReferent);
    for (size_t index = 0; index < left.size(); ++index)
        CompareTree(*left[index], *right[index], where);
}
} // namespace

TEST(XmlRobloxDocument, LoadSaveLoadPreservesTreeOnRepositoryModels) {
    const char *root = std::getenv("NOOBWARRIOR_REPO_DIR");
    if (root == nullptr)
        GTEST_SKIP() << "set NOOBWARRIOR_REPO_DIR to exercise the repository's .rbxmx assets";

    int files = 0, instances = 0;
    std::error_code code;
    for (auto it = std::filesystem::recursive_directory_iterator(root, code);
         !code && it != std::filesystem::recursive_directory_iterator(); it.increment(code)) {
        const auto ext = it->path().extension();
        if (ext != ".rbxmx" && ext != ".rbxlx")
            continue;

        std::ifstream stream(it->path(), std::ios::binary);
        const std::vector<unsigned char> bytes(
            (std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        if (bytes.empty())
            continue;

        const std::string where = it->path().filename().string();
        XmlRobloxFile first;
        std::string error;
        ASSERT_TRUE(first.Load(bytes, &error)) << where << ": " << error;

        std::vector<unsigned char> written;
        ASSERT_TRUE(first.Save(written, &error)) << where << ": " << error;

        XmlRobloxFile second;
        ASSERT_TRUE(second.Load(written, &error)) << where << " reload: " << error;

        ASSERT_EQ(first.Roots().size(), second.Roots().size()) << where << " root count";
        EXPECT_EQ(first.Metadata, second.Metadata) << where << " metadata";
        EXPECT_EQ(first.SharedStrings, second.SharedStrings) << where << " shared strings";
        for (size_t index = 0; index < first.Roots().size(); ++index)
            CompareTree(*first.Roots()[index], *second.Roots()[index], where);

        instances += static_cast<int>(first.Objects.size());
        ++files;
    }

    std::cout << "round-tripped " << files << " XML files, " << instances << " instances\n";
    EXPECT_GT(files, 0);
}

TEST(XmlRobloxDocument, KeepsTheSerializedFragmentForEachProperty) {
    const std::string xml =
        "<roblox version=\"4\"><Item class=\"Frame\" referent=\"RBX1\"><Properties>"
        "<string name=\"Name\">WindowFrame</string>"
        "<UDim2 name=\"Size\"><XS>0.5</XS><XO>10</XO><YS>0.25</YS><YO>4</YO></UDim2>"
        "</Properties></Item></roblox>";
    const std::vector<unsigned char> bytes(xml.begin(), xml.end());

    XmlRobloxFile file;
    std::string error;
    ASSERT_TRUE(file.Load(bytes, &error)) << error;
    ASSERT_EQ(1u, file.Roots().size());

    const auto &properties = file.Roots().front()->GetProperties();
    ASSERT_TRUE(properties.contains("Size"));
    const NoobWarrior::Roblox::Property &size = properties.at("Size");
    EXPECT_EQ("UDim2", size.XmlToken);
    ASSERT_FALSE(size.RawBuffer.empty()) << "the serialized fragment was dropped";
    const std::string fragment(size.RawBuffer.begin(), size.RawBuffer.end());
    EXPECT_NE(std::string::npos, fragment.find("<XS>0.5</XS>")) << fragment;
    EXPECT_NE(std::string::npos, fragment.find("<YO>4</YO>")) << fragment;
}
