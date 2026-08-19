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
// File: PluginMountingTest.cpp
// Started by: Hattozo
// Started on: 8/18/2025
// Description: Plugin mount order semantics: containers merge, everything else is replaced.
#include <NoobWarrior/PluginTreeMaterializer.h>
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryRobloxFile.h>

#include <gtest/gtest.h>

using namespace NoobWarrior;
using namespace NoobWarrior::Roblox;
using json = nlohmann::json;

namespace {
json Node(const char *name, const char *className, json children = json::array(),
          json properties = json::object()) {
    return {{"name", name}, {"className", className},
            {"children", std::move(children)}, {"properties", std::move(properties)}};
}

Instance *Child(Instance &parent, const std::string &name) {
    for (Instance *c : parent.GetChildren())
        if (c->Name == name) return c;
    return nullptr;
}

Instance *Root(BinaryFormat::BinaryRobloxFile &f, const std::string &name) {
    for (Instance *r : f.Roots())
        if (r->Name == name) return r;
    return nullptr;
}
} // namespace

TEST(PluginMounting, FoldersMergeAndNonFoldersReplace) {
    BinaryFormat::BinaryRobloxFile place;
    PluginTreeMaterializer mounter(&place);
    PluginMountResult result;
    std::string error;

    json first = json::array({Node("ServerScriptService", "ServerScriptService", json::array({
        Node("Shared", "Folder", json::array({Node("Config", "ModuleScript")})),
        Node("Thing", "Model", json::array({Node("Old", "Part")})),
    }))});
    ASSERT_TRUE(mounter.Mount(first, result, &error)) << error;

    // A second plugin contributing into the same containers.
    json second = json::array({Node("ServerScriptService", "ServerScriptService", json::array({
        Node("Shared", "Folder", json::array({Node("Extra", "ModuleScript")})),
        Node("Thing", "Model", json::array({Node("New", "Part")})),
    }))});
    ASSERT_TRUE(mounter.Mount(second, result, &error)) << error;

    Instance *service = Root(place, "ServerScriptService");
    ASSERT_NE(nullptr, service);
    EXPECT_EQ(2u, service->GetChildren().size()) << "the service must not be duplicated";

    Instance *shared = Child(*service, "Shared");
    ASSERT_NE(nullptr, shared);
    EXPECT_NE(nullptr, Child(*shared, "Config")) << "folder merge keeps the first plugin's child";
    EXPECT_NE(nullptr, Child(*shared, "Extra")) << "folder merge adds the second plugin's child";

    // A Model is not a container, so the later plugin replaces it wholesale.
    Instance *thing = Child(*service, "Thing");
    ASSERT_NE(nullptr, thing);
    EXPECT_EQ(nullptr, Child(*thing, "Old")) << "replacement must destroy the old subtree";
    ASSERT_NE(nullptr, Child(*thing, "New"));
    EXPECT_GT(result.Replaced, 0);
    EXPECT_GT(result.Merged, 0);
}

TEST(PluginMounting, MountedTreeSerializesAndReloads) {
    BinaryFormat::BinaryRobloxFile place;
    PluginTreeMaterializer mounter(&place);
    PluginMountResult result;
    std::string error;

    json plan = json::array({Node("ReplicatedStorage", "ReplicatedStorage", json::array({
        Node("Widget", "Folder", json::array({Node("Label", "TextLabel", json::array(),
             json{{"Text", "hello"}, {"Size", json{{"UDim2", json::array({json::array({0.5, 10}),
                                                                        json::array({0.25, 4})})}}}})}))
    }))});
    ASSERT_TRUE(mounter.Mount(plan, result, &error)) << error;

    std::vector<unsigned char> bytes;
    ASSERT_TRUE(place.Save(bytes, &error)) << error;

    BinaryFormat::BinaryRobloxFile reloaded;
    ASSERT_TRUE(reloaded.Load(bytes, &error)) << error;
    Instance *storage = Root(reloaded, "ReplicatedStorage");
    ASSERT_NE(nullptr, storage);
    Instance *label = Child(*Child(*storage, "Widget"), "Label");
    ASSERT_NE(nullptr, label);
    EXPECT_EQ("TextLabel", label->ClassName);
    const auto *text = label->GetProperties().at("Text").CastValue<std::string>();
    ASSERT_NE(nullptr, text);
    EXPECT_EQ("hello", *text);
    EXPECT_TRUE(label->GetProperties().contains("Size"));
}

// A Ref names another node of the same plan, which only becomes a real instance once the whole
// plan is mounted. Regression cover for those being dropped.
TEST(PluginMounting, ResolvesReferencesWithinAPlan) {
    BinaryFormat::BinaryRobloxFile place;
    PluginTreeMaterializer mounter(&place);
    PluginMountResult result;
    std::string error;

    json target = Node("Handle", "Part");
    target["referent"] = "/plugin/model.rbxmx#RBX2";
    json owner = Node("Rig", "Model", json::array({target}),
                      json{{"PrimaryPart", json{{"Ref", "/plugin/model.rbxmx#RBX2"}}}});
    json plan = json::array({Node("Workspace", "Workspace", json::array({owner}))});
    ASSERT_TRUE(mounter.Mount(plan, result, &error)) << error;

    Instance *rig = Child(*Root(place, "Workspace"), "Rig");
    ASSERT_NE(nullptr, rig);
    Instance *handle = Child(*rig, "Handle");
    ASSERT_NE(nullptr, handle);

    const auto &properties = rig->GetProperties();
    ASSERT_TRUE(properties.contains("PrimaryPart")) << "the Ref was dropped";
    EXPECT_EQ(PropertyType::Ref, properties.at("PrimaryPart").Type);
    const auto *referent = properties.at("PrimaryPart").CastValue<int32_t>();
    ASSERT_NE(nullptr, referent);
    EXPECT_EQ(handle, place.GetObject(*referent)) << "the Ref points at the wrong instance";
}

TEST(PluginMounting, WarnsAboutAnUnresolvableReference) {
    BinaryFormat::BinaryRobloxFile place;
    PluginTreeMaterializer mounter(&place);
    PluginMountResult result;
    std::string error;

    json owner = Node("Rig", "Model", json::array(),
                      json{{"PrimaryPart", json{{"Ref", "/plugin/model.rbxmx#MISSING"}}}});
    ASSERT_TRUE(mounter.Mount(json::array({owner}), result, &error)) << error;
    EXPECT_FALSE(Root(place, "Rig")->GetProperties().contains("PrimaryPart"));
    EXPECT_FALSE(result.Warnings.empty()) << "an unresolvable Ref should be reported";
}
