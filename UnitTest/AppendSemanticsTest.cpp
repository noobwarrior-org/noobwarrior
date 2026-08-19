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
// File: AppendSemanticsTest.cpp
// Started by: Hattozo
// Started on: 8/18/2025
// Description: Script routing, batching and validation for both ported file classes.
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryRobloxFile.h>
#include <NoobWarrior/Roblox/FileFormat/XmlFormat/XmlRobloxFile.h>

#include <gtest/gtest.h>

#include <string>
#include <vector>

using NoobWarrior::Roblox::Instance;
using NoobWarrior::Roblox::LuaSourceContainerSpec;
using NoobWarrior::Roblox::PropertyType;
using NoobWarrior::Roblox::BinaryFormat::BinaryRobloxFile;
using NoobWarrior::Roblox::XmlFormat::XmlRobloxFile;

namespace {
Instance *FindChild(Instance &parent, const std::string &name) {
    for (Instance *child : parent.GetChildren()) {
        if (child->Name == name)
            return child;
    }
    return nullptr;
}

template<typename File>
Instance *FindRoot(File &file, const std::string &name) {
    for (Instance *root : file.Roots()) {
        if (root->ClassName == name)
            return root;
    }
    return nullptr;
}

template<typename File>
void ExpectServerScriptRouting() {
    File file;
    const LuaSourceContainerSpec spec {"Script", "Boot", "print('hi')", true,
                                       "ServerScriptService"};
    std::string error;
    ASSERT_TRUE(file.AppendLuaSourceContainers(std::span(&spec, 1), &error)) << error;

    Instance *service = FindRoot(file, "ServerScriptService");
    ASSERT_NE(nullptr, service) << "ServerScriptService should be created when absent";
    Instance *script = FindChild(*service, "Boot");
    ASSERT_NE(nullptr, script);
    EXPECT_EQ("Script", script->ClassName);

    const auto &properties = script->GetProperties();
    ASSERT_TRUE(properties.contains("Source"));
    ASSERT_TRUE(properties.contains("Disabled"));
    EXPECT_EQ(PropertyType::ProtectedString, properties.at("Source").Type);
    const auto *disabled = properties.at("Disabled").template CastValue<bool>();
    ASSERT_NE(nullptr, disabled);
    EXPECT_TRUE(*disabled);
}

template<typename File>
void ExpectClientScriptRouting() {
    File file;
    const LuaSourceContainerSpec spec {"LocalScript", "Ui", "print('c')", false,
                                       "StarterPlayerScripts"};
    std::string error;
    ASSERT_TRUE(file.AppendLuaSourceContainers(std::span(&spec, 1), &error)) << error;

    // StarterPlayerScripts is not a root service; it must be created underneath StarterPlayer.
    Instance *starterPlayer = FindRoot(file, "StarterPlayer");
    ASSERT_NE(nullptr, starterPlayer) << "StarterPlayer should be created for a client script";
    Instance *scripts = FindChild(*starterPlayer, "StarterPlayerScripts");
    ASSERT_NE(nullptr, scripts);
    ASSERT_NE(nullptr, FindChild(*scripts, "Ui"));
    EXPECT_EQ(nullptr, FindRoot(file, "StarterPlayerScripts"))
        << "StarterPlayerScripts must not also be a root";
}

template<typename File>
void ExpectBatchReusesParentsAndValidatesFirst() {
    File file;
    const std::vector<LuaSourceContainerSpec> batch {
        {"Script", "A", "1", false, "ServerScriptService"},
        {"ModuleScript", "B", "2", false, "ServerScriptService"},
        {"LocalScript", "C", "3", false, "StarterPlayerScripts"},
    };
    std::string error;
    ASSERT_TRUE(file.AppendLuaSourceContainers(batch, &error)) << error;

    Instance *service = FindRoot(file, "ServerScriptService");
    ASSERT_NE(nullptr, service);
    EXPECT_EQ(2u, service->GetChildren().size()) << "both server entries share one service";

    Instance *module = FindChild(*service, "B");
    ASSERT_NE(nullptr, module);
    EXPECT_FALSE(module->GetProperties().contains("Disabled"))
        << "ModuleScript has no Disabled property";

    // A rejected batch must not leave earlier entries behind.
    File rejected;
    const std::vector<LuaSourceContainerSpec> bad {
        {"Script", "Good", "1", false, "ServerScriptService"},
        {"Part", "Bad", "2", false, "ServerScriptService"},
    };
    EXPECT_FALSE(rejected.AppendLuaSourceContainers(bad, &error));
    EXPECT_TRUE(rejected.Roots().empty()) << "validation must happen before any mutation";
}
} // namespace

TEST(BinaryAppendSemantics, RoutesServerScripts) { ExpectServerScriptRouting<BinaryRobloxFile>(); }
TEST(BinaryAppendSemantics, RoutesClientScripts) { ExpectClientScriptRouting<BinaryRobloxFile>(); }
TEST(BinaryAppendSemantics, BatchesAndValidates) {
    ExpectBatchReusesParentsAndValidatesFirst<BinaryRobloxFile>();
}

TEST(XmlAppendSemantics, RoutesServerScripts) { ExpectServerScriptRouting<XmlRobloxFile>(); }
TEST(XmlAppendSemantics, RoutesClientScripts) { ExpectClientScriptRouting<XmlRobloxFile>(); }
TEST(XmlAppendSemantics, BatchesAndValidates) {
    ExpectBatchReusesParentsAndValidatesFirst<XmlRobloxFile>();
}

TEST(BinaryAppendSemantics, AppendedScriptsSurviveSaveAndReload) {
    BinaryRobloxFile file;
    const std::vector<LuaSourceContainerSpec> batch {
        {"Script", "Server", "print('s')", true, "ServerScriptService"},
        {"LocalScript", "Client", "print('c')", false, "StarterPlayerScripts"},
    };
    std::string error;
    ASSERT_TRUE(file.AppendLuaSourceContainers(batch, &error)) << error;

    std::vector<unsigned char> bytes;
    ASSERT_TRUE(file.Save(bytes, &error)) << error;

    BinaryRobloxFile reloaded;
    ASSERT_TRUE(reloaded.Load(bytes, &error)) << error;
    Instance *service = FindRoot(reloaded, "ServerScriptService");
    ASSERT_NE(nullptr, service);
    Instance *script = FindChild(*service, "Server");
    ASSERT_NE(nullptr, script);
    const auto *source = script->GetProperties().at("Source")
        .CastValue<NoobWarrior::Roblox::DataTypes::ProtectedString>();
    ASSERT_NE(nullptr, source);
    EXPECT_EQ("print('s')", source->ToString());
}
