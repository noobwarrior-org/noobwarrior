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
// File: BinaryRobloxFileTest.cpp
// Started by: Hattozo
// Started on: 8/18/2025
// Description: load -> save -> load object-graph equality across installed places.
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryRobloxFile.h>

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <vector>

using NoobWarrior::Roblox::BinaryFormat::BinaryRobloxFile;
using NoobWarrior::Roblox::Instance;

namespace {
// Compares the two graphs on everything the format is required to preserve.
void ExpectSameGraph(const BinaryRobloxFile &a, const BinaryRobloxFile &b,
                     const std::string &path) {
    ASSERT_EQ(a.NumObjects, b.NumObjects) << path;
    ASSERT_EQ(a.Objects.size(), b.Objects.size()) << path;
    for (size_t index = 0; index < a.Objects.size(); ++index) {
        const Instance *left = a.Objects[index].get();
        const Instance *right = b.Objects[index].get();
        ASSERT_EQ(left == nullptr, right == nullptr) << path << " referent " << index;
        if (left == nullptr)
            continue;
        EXPECT_EQ(left->ClassName, right->ClassName) << path << " referent " << index;
        EXPECT_EQ(left->Name, right->Name) << path << " referent " << index;
        EXPECT_EQ(left->GetProperties().size(), right->GetProperties().size())
            << path << " " << left->ClassName << " referent " << index;
        for (const auto &[name, property] : left->GetProperties()) {
            const auto found = right->GetProperties().find(name);
            ASSERT_NE(right->GetProperties().end(), found)
                << path << " lost " << left->ClassName << "." << name;
            EXPECT_EQ(property.Type, found->second.Type) << path << " " << name;
        }
        const Instance *leftParent = const_cast<Instance *>(left)->GetParent();
        const Instance *rightParent = const_cast<Instance *>(right)->GetParent();
        ASSERT_EQ(leftParent == nullptr, rightParent == nullptr) << path << " parent " << index;
        if (leftParent != nullptr)
            EXPECT_EQ(leftParent->Referent, rightParent->Referent) << path << " parent " << index;
    }
}
} // namespace

TEST(BinaryRobloxDocument, LoadSaveLoadPreservesGraphOnInstalledPlaces) {
    const char *root = std::getenv("NOOBWARRIOR_ENGINES_DIR");
    if (root == nullptr)
        GTEST_SKIP() << "set NOOBWARRIOR_ENGINES_DIR to cross-check against installed places";

    int places = 0, objects = 0, failures = 0;
    uint64_t before = 0, after = 0;
    std::error_code code;
    for (auto it = std::filesystem::recursive_directory_iterator(root, code);
         !code && it != std::filesystem::recursive_directory_iterator(); it.increment(code)) {
        if (it->path().extension() != ".rbxl")
            continue;
        std::ifstream s(it->path(), std::ios::binary);
        const std::vector<unsigned char> bytes(
            (std::istreambuf_iterator<char>(s)), std::istreambuf_iterator<char>());
        if (bytes.empty())
            continue;

        BinaryRobloxFile first;
        std::string error;
        if (!first.Load(bytes, &error)) {
            ++failures;
            ADD_FAILURE() << "load failed: " << error << " for " << it->path().string();
            continue;
        }

        std::vector<unsigned char> written;
        if (!first.Save(written, &error)) {
            ++failures;
            ADD_FAILURE() << "save failed: " << error << " for " << it->path().string();
            continue;
        }

        BinaryRobloxFile second;
        if (!second.Load(written, &error)) {
            ++failures;
            ADD_FAILURE() << "reload failed: " << error << " for " << it->path().string();
            continue;
        }

        ExpectSameGraph(first, second, it->path().string());
        before += bytes.size();
        after += written.size();
        objects += static_cast<int>(first.NumObjects);
        ++places;
    }

    std::cout << "round-tripped " << places << " places, " << objects << " objects; "
              << before << " -> " << after << " bytes ("
              << (before == 0 ? 0 : after * 100 / before) << "%), "
              << failures << " failures\n";
    EXPECT_GT(places, 0);
    EXPECT_EQ(0, failures);
}
