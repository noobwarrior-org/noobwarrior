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
// File: PluginDataModelTest.cpp
// Started by: Hattozo
// Started on: 8/18/2025
// Description: Plugin RBXMX models are materialized without Studio plugin capabilities.
#include <NoobWarrior/FileSystem/VirtualFileSystem.h>
#include <NoobWarrior/PluginDataModel.h>

#include <gtest/gtest.h>

#include <chrono>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace {
class TemporaryPluginDirectory {
public:
    TemporaryPluginDirectory() {
        const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
        Path = std::filesystem::temp_directory_path() /
            ("noobwarrior-rbxmx-test-" + std::to_string(suffix));
        std::filesystem::create_directories(Path / "datamodel");
    }

    ~TemporaryPluginDirectory() {
        std::error_code error;
        std::filesystem::remove_all(Path, error);
    }

    std::filesystem::path Path;
};


template<typename T>
void Append(std::vector<unsigned char> &data, T value) {
    const auto *bytes = reinterpret_cast<const unsigned char *>(&value);
    data.insert(data.end(), bytes, bytes + sizeof(value));
}

void AppendString(std::vector<unsigned char> &data, std::string_view value) {
    Append(data, static_cast<uint32_t>(value.size()));
    data.insert(data.end(), value.begin(), value.end());
}

void AppendChunk(std::vector<unsigned char> &file, std::string_view type,
                 const std::vector<unsigned char> &data) {
    for (size_t index = 0; index < 4; ++index)
        file.push_back(index < type.size() ? type[index] : 0);
    Append(file, uint32_t {0});
    Append(file, static_cast<uint32_t>(data.size()));
    Append(file, uint32_t {0});
    file.insert(file.end(), data.begin(), data.end());
}

std::vector<unsigned char> MinimalBinaryModel() {
    constexpr std::array<unsigned char, 14> magic = {
        '<', 'r', 'o', 'b', 'l', 'o', 'x', '!', 0x89, 0xff, 0x0d, 0x0a, 0x1a, 0x0a,
    };
    std::vector<unsigned char> file(magic.begin(), magic.end());
    Append(file, uint16_t {0});
    Append(file, uint32_t {1});
    Append(file, uint32_t {1});
    Append(file, uint64_t {0});

    std::vector<unsigned char> instances;
    Append(instances, uint32_t {0});
    AppendString(instances, "Folder");
    instances.push_back(0);
    Append(instances, uint32_t {1});
    Append(instances, uint32_t {0});
    AppendChunk(file, "INST", instances);

    std::vector<unsigned char> names;
    Append(names, uint32_t {0});
    AppendString(names, "Name");
    names.push_back(0x01);
    AppendString(names, "BinaryRoot");
    AppendChunk(file, "PROP", names);

    std::vector<unsigned char> parents {0};
    Append(parents, uint32_t {1});
    Append(parents, uint32_t {0});
    parents.insert(parents.end(), {0, 0, 0, 1});
    AppendChunk(file, "PRNT", parents);
    const std::string_view ending = "</roblox>";
    AppendChunk(file, "END", std::vector<unsigned char>(ending.begin(), ending.end()));
    return file;
}
} // namespace

TEST(PluginDataModel, BinaryRbxmIsCarriedInBootstrapWithoutHttpTransport) {
    TemporaryPluginDirectory plugin;
    const std::vector<unsigned char> model = MinimalBinaryModel();
    std::filesystem::create_directories(
        plugin.Path / "datamodel" / "ServerScriptService");
    std::ofstream output(plugin.Path / "datamodel" / "ServerScriptService" /
                         "model.rbxm", std::ios::binary);
    ASSERT_TRUE(output.is_open());
    output.write(reinterpret_cast<const char *>(model.data()),
                 static_cast<std::streamsize>(model.size()));
    output.close();

    NoobWarrior::VirtualFileSystem *rawVfs = nullptr;
    ASSERT_EQ(NoobWarrior::VirtualFileSystem::Response::Success,
              NoobWarrior::VirtualFileSystem::New(&rawVfs, plugin.Path));
    std::unique_ptr<NoobWarrior::VirtualFileSystem,
                    decltype(&NoobWarrior::VirtualFileSystem::Free)>
        vfs(rawVfs, &NoobWarrior::VirtualFileSystem::Free);

    NoobWarrior::PluginDataModel builder(vfs.get(), "binary-test@example.com");
    NoobWarrior::StudioServerBootstrap bootstrap = builder.BuildBootstrap("/datamodel");
    const std::string plan = bootstrap.Plans.empty() ? std::string() : bootstrap.Plans.front();
    ASSERT_EQ(1u, bootstrap.Models.size());
    EXPECT_EQ(model, bootstrap.Models.front().Data);
    EXPECT_EQ("/datamodel/ServerScriptService/model.rbxm",
              bootstrap.Models.front().SourcePath);
    ASSERT_EQ(1u, bootstrap.Models.front().ParentPath.size());
    EXPECT_EQ("ServerScriptService", bootstrap.Models.front().ParentPath.front().Name);
    EXPECT_EQ("ServerScriptService", bootstrap.Models.front().ParentPath.front().ClassName);
    EXPECT_NE(std::string::npos, plan.find("embeddedModel"));
    EXPECT_EQ(std::string::npos,
              plan.find("game:GetService(\"ServerStorage\")"));
    EXPECT_EQ(std::string::npos, plan.find("plugin-content"));
    EXPECT_EQ(std::string::npos, plan.find("GetObjects"));
}

TEST(PluginDataModel, RbxmxIsEmbeddedInMaterializationPlan) {
    TemporaryPluginDirectory plugin;
    std::ofstream model(plugin.Path / "datamodel" / "model.rbxmx",
                        std::ios::binary);
    ASSERT_TRUE(model.is_open());
    model << R"XML(<roblox version="4">
<Item class="Model" referent="RBX1"><Properties>
<string name="Name">ModelRoot</string><Ref name="PrimaryPart">RBX2</Ref>
<bool name="NeedsPivotMigration">false</bool><token name="LevelOfDetail">0</token>
</Properties><Item class="Part" referent="RBX2"><Properties>
<string name="Name">Part0</string><bool name="Anchored">true</bool>
<Vector3 name="size"><X>2</X><Y>3</Y><Z>4</Z></Vector3>
<token name="shape">1</token><int name="formFactorRaw">0</int>
<string name="MaterialVariantSerialized">SmoothPlastic</string>
<CoordinateFrame name="CFrame"><X>1</X><Y>2</Y><Z>3</Z>
<R00>1</R00><R01>0</R01><R02>0</R02><R10>0</R10><R11>1</R11>
<R12>0</R12><R20>0</R20><R21>0</R21><R22>1</R22></CoordinateFrame>
</Properties></Item><Item class="Humanoid" referent="RBX3"><Properties>
<string name="Name">Humanoid</string><float name="Health_XML">75</float>
<float name="InternalBodyScale">1</float><token name="CollisionType">0</token>
</Properties></Item><Item class="UICorner" referent="RBX4"><Properties>
<string name="Name">Corner</string><bool name="DefinesCapabilities">false</bool>
<bool name="Sandboxed">false</bool>
<UDim name="BottomLeftRadius"><S>0</S><O>8</O></UDim>
<UDim name="BottomRightRadius"><S>0</S><O>8</O></UDim>
<UDim name="TopLeftRadius"><S>0</S><O>8</O></UDim>
<UDim name="TopRightRadius"><S>0</S><O>8</O></UDim>
</Properties></Item><Item class="TextLabel" referent="RBX5"><Properties>
<string name="Name">Title</string>
<Font name="FontFace"><Family><url>rbxasset://fonts/families/SourceSansPro.json</url></Family>
<Weight>400</Weight><Style>Normal</Style></Font>
<string name="LocalizationMatchIdentifier"></string>
<string name="LocalizationMatchedSourceText"></string>
</Properties></Item></Item></roblox>)XML";
    model.close();

    NoobWarrior::VirtualFileSystem *rawVfs = nullptr;
    ASSERT_EQ(NoobWarrior::VirtualFileSystem::Response::Success,
              NoobWarrior::VirtualFileSystem::New(&rawVfs, plugin.Path));
    std::unique_ptr<NoobWarrior::VirtualFileSystem,
                    decltype(&NoobWarrior::VirtualFileSystem::Free)>
        vfs(rawVfs, &NoobWarrior::VirtualFileSystem::Free);

    NoobWarrior::PluginDataModel builder(vfs.get(), "model-test@example.com");
    NoobWarrior::StudioServerBootstrap bootstrap = builder.BuildBootstrap("/datamodel");
    const std::string plan = bootstrap.Plans.empty() ? std::string() : bootstrap.Plans.front();
    ASSERT_FALSE(bootstrap.Plans.empty());
    EXPECT_NE(std::string::npos, plan.find("ModelRoot"));
    EXPECT_NE(std::string::npos, plan.find("Part0"));
    EXPECT_NE(std::string::npos, plan.find("PrimaryPart"));
    EXPECT_NE(std::string::npos, plan.find("/datamodel/model.rbxmx#RBX2"));
    EXPECT_NE(std::string::npos, plan.find("\"Size\":"));
    EXPECT_NE(std::string::npos, plan.find("\"Shape\":"));
    EXPECT_NE(std::string::npos, plan.find("\"MaterialVariant\":"));
    EXPECT_NE(std::string::npos, plan.find("\"Health\":"));
    EXPECT_NE(std::string::npos, plan.find("\"CornerRadius\":"));
    EXPECT_NE(std::string::npos, plan.find("\"FontFace\":{\"Font\":"));
    EXPECT_NE(std::string::npos,
              plan.find("rbxasset://fonts/families/SourceSansPro.json"));
    EXPECT_EQ(std::string::npos, plan.find("\"size\":"));
    EXPECT_EQ(std::string::npos, plan.find("NeedsPivotMigration"));
    EXPECT_EQ(std::string::npos, plan.find("LevelOfDetail"));
    EXPECT_EQ(std::string::npos, plan.find("formFactorRaw"));
    EXPECT_EQ(std::string::npos, plan.find("InternalBodyScale"));
    EXPECT_EQ(std::string::npos, plan.find("CollisionType"));
    EXPECT_EQ(std::string::npos, plan.find("DefinesCapabilities"));
    EXPECT_EQ(std::string::npos, plan.find("\"Sandboxed\":"));
    EXPECT_EQ(std::string::npos, plan.find("BottomLeftRadius"));
    EXPECT_EQ(std::string::npos, plan.find("BottomRightRadius"));
    EXPECT_EQ(std::string::npos, plan.find("TopLeftRadius"));
    EXPECT_EQ(std::string::npos, plan.find("TopRightRadius"));
    EXPECT_EQ(std::string::npos, plan.find("LocalizationMatchIdentifier"));
    EXPECT_EQ(std::string::npos, plan.find("LocalizationMatchedSourceText"));
    EXPECT_NE(std::string::npos, plan.find("\"rbxmxBestEffort\":true"));
    EXPECT_EQ(std::string::npos, plan.find("plugin-content"));
    EXPECT_EQ(std::string::npos, plan.find("model.rbxmx\""));
}

TEST(PluginDataModel, RbxmxOmitsAsymmetricProtectedUICornerProperties) {
    TemporaryPluginDirectory plugin;
    std::ofstream model(plugin.Path / "datamodel" / "corner.rbxmx",
                        std::ios::binary);
    ASSERT_TRUE(model.is_open());
    model << R"XML(<roblox version="4">
<Item class="UICorner" referent="RBX1"><Properties>
<string name="Name">AsymmetricCorner</string>
<UDim name="BottomLeftRadius"><S>0</S><O>1</O></UDim>
<UDim name="BottomRightRadius"><S>0</S><O>2</O></UDim>
<UDim name="TopLeftRadius"><S>0</S><O>3</O></UDim>
<UDim name="TopRightRadius"><S>0</S><O>4</O></UDim>
</Properties></Item></roblox>)XML";
    model.close();
    std::ofstream project(plugin.Path / "datamodel" / "default.project.json",
                          std::ios::binary);
    ASSERT_TRUE(project.is_open());
    project << R"JSON({"name":"CornerProject","tree":{"$path":"corner.rbxmx","$properties":{"DefinitelyNotAProperty":true}}})JSON";
    project.close();

    NoobWarrior::VirtualFileSystem *rawVfs = nullptr;
    ASSERT_EQ(NoobWarrior::VirtualFileSystem::Response::Success,
              NoobWarrior::VirtualFileSystem::New(&rawVfs, plugin.Path));
    std::unique_ptr<NoobWarrior::VirtualFileSystem,
                    decltype(&NoobWarrior::VirtualFileSystem::Free)>
        vfs(rawVfs, &NoobWarrior::VirtualFileSystem::Free);

    NoobWarrior::PluginDataModel builder(vfs.get(), "corner-test@example.com");
    NoobWarrior::StudioServerBootstrap bootstrap = builder.BuildBootstrap("/datamodel");
    const std::string plan = bootstrap.Plans.empty() ? std::string() : bootstrap.Plans.front();
    ASSERT_FALSE(bootstrap.Plans.empty());
    EXPECT_EQ(std::string::npos, plan.find("\"BottomLeftRadius\":"));
    EXPECT_EQ(std::string::npos, plan.find("\"BottomRightRadius\":"));
    EXPECT_EQ(std::string::npos, plan.find("\"TopLeftRadius\":"));
    EXPECT_EQ(std::string::npos, plan.find("\"TopRightRadius\":"));
    EXPECT_EQ(std::string::npos, plan.find("\"CornerRadius\":"));
    EXPECT_NE(std::string::npos,
              plan.find("\"strictProperties\":{\"DefinitelyNotAProperty\":true}"));
}
