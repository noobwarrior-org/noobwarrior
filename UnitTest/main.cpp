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
// File: main.cpp (UnitTest)
// Started by: Hattozo
// Started on: 2/16/2026
// Description: Main file for unit testing
#include <gtest/gtest.h>
#include <NoobWarrior.hpp>

#include <stdlib.h>

using namespace NoobWarrior;

static Init sInit {};
static NoobWarrior::Core* sCore;
static EmuDb* sEmuDb;

TEST(Core, Init) {
    sCore = new Core(sInit);
    EXPECT_EQ(sCore->Fail(), false) << "noobWarrior failed to initialize. You can assume the test is over now.";
}

TEST(Url, GetProtocol) {
    Url url("https://youtube.com");
    EXPECT_EQ(ProtocolType::Https, url.GetProtocol())
        << "Url::GetProtocol() returned wrong value for URL \"https://youtube.com\", check the quality of this function.";
}

TEST(Url, GetProtocolString) {
    Url url("https://youtube.com");
    EXPECT_EQ("https", url.GetProtocolString())
        << "Url::GetProtocolString() returned wrong value for URL \"https://youtube.com\", check the quality of this function.";
}

TEST(Url, GetHostNameForWebsite) {
    Url url("https://youtube.com");
    EXPECT_EQ("youtube.com", url.GetHostName())
        << "Url::GetHostName() returned wrong value for URL \"https://youtube.com\", check the quality of this function.";
}

TEST(Url, GetHostNameForPlugin) {
    Url url("plugin://emu-frontend@noobwarrior.org/lua/main.lua");
    EXPECT_EQ("emu-frontend@noobwarrior.org", url.GetHostName())
        << "Url::GetHostName() returned wrong value for URL \"plugin://emu-frontend@noobwarrior.org/lua/main.lua\", check the quality of this function.";
}

TEST(Url, ResolveAlreadyAbsolutePath) {
    Url url("userdata://databases/master.nwdb");
    EXPECT_EQ("userdata://databases/master.nwdb", url.Resolve())
        << "userdata://databases/master.nwdb did not resolve to the correct URL. Check the quality of Url::Resolve().";
}

TEST(Url, ResolveWithoutProtocol) {
    Url url("https://youtube.com/watch?v=jNQXAC9IVRw");
    EXPECT_EQ("youtube.com/watch?v=jNQXAC9IVRw", url.ResolveWithoutProtocol())
        << "Url::ResolveWithoutProtocol() returned wrong value for URL \"https://youtube.com/watch?v=jNQXAC9IVRw\", check the quality of this function.";
}

TEST(Url, ResolveAsPathName) {
    Url url("https://youtube.com/watch?v=jNQXAC9IVRw");
    EXPECT_EQ("/watch?v=jNQXAC9IVRw", url.ResolveAsPath())
        << "Url::ResolveAsPathName() returned wrong value for URL \"https://youtube.com/watch?v=jNQXAC9IVRw\", check the quality of this function.";
}

TEST(Url, ResolveUsingContext) {
    Url url("lua/main.lua", {
        .DefaultProtocolType = ProtocolType::Plugin,
        .DefaultHostName = "emu-frontend@noobwarrior.org"
    });
    EXPECT_EQ("plugin://emu-frontend@noobwarrior.org/lua/main.lua", url.Resolve())
        << "lua/main.lua did not resolve to the correct URL. Check the quality of Url::Resolve().";
}

TEST(Url, ResolveAsPathNameUsingContext) {
    Url url("lua/main.lua", {
        .DefaultProtocolType = ProtocolType::Plugin,
        .DefaultHostName = "emu-frontend@noobwarrior.org"
    });
    EXPECT_EQ("/lua/main.lua", url.ResolveAsPath())
        << "plugin://emu-frontend@noobwarrior.org/lua/main.lua did not resolve to /lua/main.lua using url.ResolveAsPathName(). Check the quality of Url::ResolveAsPathName() and Url::Resolve().";
}

TEST(Url, ResolveUsingContextForWebsiteWithoutHttpsSpecifier) {
    Url url("youtube.com/watch?v=jNQXAC9IVRw", {
        .DefaultProtocolType = ProtocolType::Https
    });
    EXPECT_EQ("https://youtube.com/watch?v=jNQXAC9IVRw", url.Resolve())
        << "youtube.com/watch?v=jNQXAC9IVRw did not resolve to the correct URL. Check the quality of Url::Resolve().";
}

TEST(Url, EnforceCorrectProtocol) {
    Url url("https://youtube.com", {
        .DefaultProtocolType = ProtocolType::File,
        .EnforceProtocolType = true
    });
    EXPECT_EQ(true, url.Fail())
        << "Url should fail with incorrect protocol, but it's not.";
}

TEST(Url, EnforceCorrectHostName) {
    Url url("https://youtube.com", {
        .DefaultHostName = "example.com",
        .EnforceHostName = true
    });
    EXPECT_EQ(true, url.Fail())
        << "Url should fail with incorrect host name, but it's not.";
}

TEST(Url, EnforceCorrectProtocolAndHostName) {
    Url url("https://youtube.com", {
        .DefaultProtocolType = ProtocolType::File,
        .DefaultHostName = "example.com",
        .EnforceProtocolType = true,
        .EnforceHostName = true
    });
    EXPECT_EQ(true, url.Fail())
        << "Url should fail with incorrect protocol and host name, but it's not.";
}

TEST(Vfs, Initialize) {
    VirtualFileSystem* vfs;
#ifdef _WIN32
    VirtualFileSystem::Response res = VirtualFileSystem::New(&vfs, "C:\\Windows");
#else
    VirtualFileSystem::Response res = VirtualFileSystem::New(&vfs, "/usr");
#endif
    VirtualFileSystem::Free(vfs);
    EXPECT_EQ(VirtualFileSystem::Response::Success, res)
        << "VFS failed to initialize!";
}

TEST(Vfs, GetEntriesInDirectory) {
    VirtualFileSystem* vfs;
#ifdef _WIN32
    VirtualFileSystem::Response res = VirtualFileSystem::New(&vfs, "C:\\Windows");
#else
    VirtualFileSystem::Response res = VirtualFileSystem::New(&vfs, "/usr");
#endif
    std::vector<FSEntryInfo> entries = vfs->GetEntriesInDirectory("/");
    EXPECT_EQ(false, entries.empty())
        << "Entries for directory mounted using OverlayFS should not be empty, but it is.";
    VirtualFileSystem::Free(vfs);
}

TEST(Vfs, OverlayFs) {
    auto *vfs = new OverlayFileSystem();
#ifdef _WIN32
    VirtualFileSystem::Response res = vfs->Mount("/", "C:\\Windows");
#else
    VirtualFileSystem::Response res = vfs->Mount("/", "/usr");
#endif
    EXPECT_EQ(VirtualFileSystem::Response::Success, res)
        << "OverlayFS failed to mount!";

    std::vector<FSEntryInfo> entries = vfs->GetEntriesInDirectory("/");
    for (FSEntryInfo &entry : entries) {
        Out("OverlayFileSystem", "{}", entry.Name);
    }
    EXPECT_EQ(false, entries.empty())
        << "Entries for directory mounted using OverlayFS should not be empty, but it is.";
    delete vfs;
}

#define RUN_LUA(src) \
    LuaScript lua(sCore->GetLuaState(), sCore->GetLuaState()->globals(), src); \
    EXPECT_EQ(false, lua.Fail()) \
        << "Failed to load Lua script!"; \
    LuaScript::ExecResponse res = lua.Execute(); \
    EXPECT_EQ(LuaScript::ExecResponse::Ok, res) \
        << "Failed to execute Lua script!";

TEST(Lua, RunScript) {
    RUN_LUA("print(\"Hello from UnitTest!\")")
}

TEST(Lua, PrintMultipleArgs) {
    RUN_LUA("print(\"This is the first arg!\", \"This is the second arg!\")")
}

// BTW: This triggers a prompt from Windows Firewall as it starts a HTTP server.
TEST(Lua, CreateHttpServerFromScript) {
    RUN_LUA("local server = HttpServer.new() server:Start(43000) print(\"Created HttpServer from script! Memory Address:\", server) server:Stop()")
}

TEST(Lua, RenderLhpPage) {
    RUN_LUA("print(lhp.Render(\"Hello, this is plain text. <?lua echo('And this is from LHP!') ?>\"))")
}

TEST(Lua, Signal) {
    RUN_LUA("local signal = Signal.new() signal:Connect(function() print(\"Oh hello from a signal listener!\") end) signal:Fire()")
}

TEST(Lua, SignalMultiple) {
    RUN_LUA("local signal = Signal.new() local s1 = signal:Connect(function() print(\"Hello from the first signal listener!\") end) local s2 = signal:Connect(function() print(\"Hello from the second signal listener!\") end) signal:Fire()")
}

TEST(Lua, SignalParameter) {
    RUN_LUA("local signal = Signal.new() signal:Connect(function(msg) print('Msg sent from fired signal: \"'..msg..'\"') end) signal:Fire(\"Hello from fired signal!\")")
}

TEST(Database, Open) {
    sEmuDb = new EmuDb(":memory:");
    EXPECT_EQ(false, sEmuDb->Fail());
}

TEST(Database, AddBlob) {
    SqlDb::Response res = sEmuDb->AddBlob({'t', 'e', 's', 't', '\0'});
    EXPECT_EQ(SqlDb::Response::Success, res)
        << "Failed to insert a row in the BlobStorage table. Check the quality of the EmuDb::AddBlob() function.";
}

TEST(Database, AddAsset) {
    SqlDb::Response res = sEmuDb->AddItem(ItemType::Asset, {
        {"Id", 1},
        {"Name", "Test"},
        {"Description", "Test Description"},
        {"Type", static_cast<int>(Roblox::AssetType::Model)}
    });
    EXPECT_EQ(SqlDb::Response::Success, res)
        << "Failed to insert a row in the Asset table with an ID of 1. Check the quality of the EmuDb::AddItem() function.";
}

TEST(Database, UpdateAsset) {
    SqlDb::Response res = sEmuDb->UpdateItem(ItemType::Asset, 1, {
        {"Name", "My New Test Name"},
        {"Description", "My New Test Description"},
        {"Type", static_cast<int>(Roblox::AssetType::Place)}
    });
    EXPECT_EQ(SqlDb::Response::Success, res)
        << "Failed to update a few columns for asset ID 1. Check the quality of the EmuDb::UpdateItem() function.";
}

TEST(Database, AttachDataToAsset) {
    SqlDb::Response res = sEmuDb->AttachDataToAsset(1, 0, {'h', 'e', 'l', 'l', 'o', '\0'});
    EXPECT_EQ(SqlDb::Response::Success, res)
        << "Failed to add data for asset ID 1. Check the quality of the EmuDb::AttachDataToAsset() function.";
}

TEST(Database, UpdateAttachDataToAsset) {
    SqlDb::Response res = sEmuDb->AttachDataToAsset(1, 0, {'n', 'e', 'w', ' ', 'h', 'e', 'l', 'l', 'o', '\0'});
    EXPECT_EQ(SqlDb::Response::Success, res)
        << "Failed to update the data for asset ID 1. Check the quality of the EmuDb::AttachDataToAsset() function.";
}

TEST(Database, DeleteAsset) {
    SqlDb::Response res = sEmuDb->DeleteItem(ItemType::Asset, 1);
    EXPECT_EQ(SqlDb::Response::Success, res)
        << "Failed to delete the data for asset ID 1. Check the quality of the EmuDb::DeleteItem() function.";;
}

TEST(Database, Close) {
    delete sEmuDb;
}

int main(int argc, char** argv) {
    #if defined(_WIN32)
        _putenv("GTEST_COLOR=yes");
        _putenv("GTEST_OUTPUT=xml:results.xml");
    #else
        setenv("GTEST_COLOR", "yes", 0);
        setenv("GTEST_OUTPUT", "xml:results.xml", 0);
    #endif

    sInit.ArgCount = argc;
    sInit.ArgVec = argv;
    sInit.Portable = true;
    
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
