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
// File: AppendBinaryModelTest.cpp
// Started by: Hattozo
// Started on: 8/18/2025
// Description: Merging an RBXM tree into a document, with referent and Ref remapping.
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryRobloxFile.h>

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <string>
#include <vector>

using NoobWarrior::Roblox::Instance;
using NoobWarrior::Roblox::PropertyType;
using NoobWarrior::Roblox::BinaryFormat::BinaryModelPathElement;
using NoobWarrior::Roblox::BinaryFormat::BinaryRobloxFile;

namespace {
std::vector<unsigned char> Read(const std::filesystem::path &path) {
    std::ifstream stream(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

std::filesystem::path FirstPlace(const char *root) {
    std::error_code code;
    for (auto it = std::filesystem::recursive_directory_iterator(root, code);
         !code && it != std::filesystem::recursive_directory_iterator(); it.increment(code)) {
        if (it->path().extension() == ".rbxl" && std::filesystem::file_size(it->path()) > 200000)
            return it->path();
    }
    return {};
}
} // namespace

namespace {
// One root named Widget with a single child, so two of these collide by name at a destination.
std::vector<unsigned char> BuildWidgetModel(const std::string &childName) {
    BinaryRobloxFile model;
    Instance *root = model.CreateInstance("Model", "Widget", nullptr);
    EXPECT_NE(nullptr, root);
    model.CreateInstance("Part", childName, root);
    std::vector<unsigned char> bytes;
    std::string error;
    EXPECT_TRUE(model.Save(bytes, &error)) << error;
    return bytes;
}

Instance *FindChild(Instance *parent, const std::string &name) {
    for (Instance *child : parent->GetChildren()) {
        if (child->Name == name)
            return child;
    }
    return nullptr;
}
} // namespace

// A model file is one replaceable unit: the later plugin's copy wins outright rather than landing
// beside the earlier one.
TEST(AppendBinaryModel, LaterModelReplacesAnEarlierOneWithTheSameRoot) {
    const std::vector<unsigned char> first = BuildWidgetModel("First");
    const std::vector<unsigned char> second = BuildWidgetModel("Second");

    BinaryRobloxFile destination;
    const std::vector<BinaryModelPathElement> path {
        {"ServerScriptService", "ServerScriptService"}};
    std::string error;

    size_t replaced = 0;
    ASSERT_TRUE(destination.AppendBinaryModel(first, path, {}, &error, &replaced)) << error;
    EXPECT_EQ(0u, replaced);
    ASSERT_TRUE(destination.AppendBinaryModel(second, path, {}, &error, &replaced)) << error;
    EXPECT_EQ(1u, replaced);

    Instance *service = FindChild(&destination, "ServerScriptService");
    ASSERT_NE(nullptr, service);
    int widgets = 0;
    for (Instance *child : service->GetChildren()) {
        if (child->Name == "Widget")
            ++widgets;
    }
    EXPECT_EQ(1, widgets);

    Instance *widget = FindChild(service, "Widget");
    ASSERT_NE(nullptr, widget);
    EXPECT_NE(nullptr, FindChild(widget, "Second"));
    EXPECT_EQ(nullptr, FindChild(widget, "First"));
}

// Roots that do not collide still accumulate.
TEST(AppendBinaryModel, DistinctRootsCoexist) {
    const std::vector<unsigned char> model = BuildWidgetModel("Child");
    BinaryRobloxFile destination;
    const std::vector<BinaryModelPathElement> path {
        {"ServerScriptService", "ServerScriptService"}};
    std::string error;

    size_t replaced = 0;
    ASSERT_TRUE(destination.AppendBinaryModel(model, path, "AAA", &error, &replaced)) << error;
    ASSERT_TRUE(destination.AppendBinaryModel(model, path, "BBB", &error, &replaced)) << error;
    EXPECT_EQ(0u, replaced);

    Instance *service = FindChild(&destination, "ServerScriptService");
    ASSERT_NE(nullptr, service);
    EXPECT_NE(nullptr, FindChild(service, "AAA"));
    EXPECT_NE(nullptr, FindChild(service, "BBB"));
}

// Destroying an instance leaves a hole in the object list, and referents are positions in that
// list, so a place that had anything replaced has to compact before it can be read back.
TEST(AppendBinaryModel, APlaceStillLoadsAfterAReplacement) {
    BinaryRobloxFile destination;
    const std::vector<BinaryModelPathElement> path {
        {"ServerScriptService", "ServerScriptService"}};
    std::string error;
    ASSERT_TRUE(destination.AppendBinaryModel(BuildWidgetModel("First"), path, {}, &error))
        << error;
    ASSERT_TRUE(destination.AppendBinaryModel(BuildWidgetModel("Second"), path, {}, &error))
        << error;

    std::vector<unsigned char> saved;
    ASSERT_TRUE(destination.Save(saved, &error)) << error;

    BinaryRobloxFile reloaded;
    ASSERT_TRUE(reloaded.Load(saved, &error)) << error;
    Instance *service = FindChild(&reloaded, "ServerScriptService");
    ASSERT_NE(nullptr, service);
    Instance *widget = FindChild(service, "Widget");
    ASSERT_NE(nullptr, widget);
    EXPECT_NE(nullptr, FindChild(widget, "Second"));
}

TEST(AppendBinaryModel, MergesModelTreeAndRemapsReferents) {
    const char *root = std::getenv("NOOBWARRIOR_ENGINES_DIR");
    if (root == nullptr)
        GTEST_SKIP() << "set NOOBWARRIOR_ENGINES_DIR";
    const std::filesystem::path place = FirstPlace(root);
    ASSERT_FALSE(place.empty());

    BinaryRobloxFile source;
    std::string error;
    ASSERT_TRUE(source.Load(Read(place), &error)) << error;
    std::vector<unsigned char> modelBytes;
    ASSERT_TRUE(source.Save(modelBytes, &error)) << error;
    const uint32_t sourceObjects = source.NumObjects;
    ASSERT_GT(sourceObjects, 100u);

    BinaryRobloxFile destination;
    const std::vector<BinaryModelPathElement> path {
        {"ServerScriptService", "ServerScriptService"}, {"Plugins", "Folder"}};
    ASSERT_TRUE(destination.AppendBinaryModel(modelBytes, path, {}, &error)) << error;

    // The two path nodes plus every source object.
    EXPECT_EQ(sourceObjects + 2, destination.Objects.size());

    // Referents must be unique and every Ref must resolve inside the destination.
    std::set<std::string> referents;
    int refs = 0, resolved = 0;
    for (const auto &object : destination.Objects) {
        ASSERT_NE(nullptr, object);
        EXPECT_TRUE(referents.insert(object->Referent).second)
            << "duplicate referent " << object->Referent;
        for (const auto &[name, property] : object->GetProperties()) {
            if (property.Type != PropertyType::Ref)
                continue;
            ++refs;
            const auto *value = property.CastValue<int32_t>();
            ASSERT_NE(nullptr, value);
            if (*value < 0)
                continue;
            EXPECT_NE(nullptr, destination.GetObject(*value)) << "dangling Ref " << name;
            ++resolved;
        }
    }
    EXPECT_GT(refs, 0) << "the sample place should contain Ref properties";

    // The model's roots hang off the requested path.
    Instance *service = nullptr;
    for (Instance *child : destination.Roots()) {
        if (child->Name == "ServerScriptService")
            service = child;
    }
    ASSERT_NE(nullptr, service);
    ASSERT_EQ(1u, service->GetChildren().size());
    EXPECT_EQ("Plugins", service->GetChildren().front()->Name);
    EXPECT_GT(service->GetChildren().front()->GetChildren().size(), 0u);

    // And the merged document still serializes and reloads.
    std::vector<unsigned char> written;
    ASSERT_TRUE(destination.Save(written, &error)) << error;
    BinaryRobloxFile reloaded;
    ASSERT_TRUE(reloaded.Load(written, &error)) << error;
    EXPECT_EQ(destination.Objects.size(), reloaded.Objects.size());

    std::cout << "merged " << sourceObjects << " objects, " << refs << " Ref properties ("
              << resolved << " resolved)\n";
}

TEST(AppendBinaryModel, RenamesASingleRootAndRejectsMultiple) {
    const char *root = std::getenv("NOOBWARRIOR_ENGINES_DIR");
    if (root == nullptr)
        GTEST_SKIP() << "set NOOBWARRIOR_ENGINES_DIR";

    BinaryRobloxFile scratch;
    std::string error;
    const NoobWarrior::Roblox::LuaSourceContainerSpec spec {
        "Script", "OnlyRoot", "print('x')", false, "ServerScriptService"};
    ASSERT_TRUE(scratch.AppendLuaSourceContainers(std::span(&spec, 1), &error)) << error;
    std::vector<unsigned char> bytes;
    ASSERT_TRUE(scratch.Save(bytes, &error)) << error;

    // ServerScriptService is the only root, so renaming is well defined.
    BinaryRobloxFile destination;
    ASSERT_TRUE(destination.AppendBinaryModel(bytes, {}, "Renamed", &error)) << error;
    bool found = false;
    for (Instance *child : destination.Roots())
        found = found || child->Name == "Renamed";
    EXPECT_TRUE(found) << "single root was not renamed";
}
