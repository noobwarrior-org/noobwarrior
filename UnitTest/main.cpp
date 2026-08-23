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
#include <cpr/cpr.h>

#include <gtest/gtest.h>
#include <NoobWarrior.hpp>
#include <NoobWarrior/HttpServer/Emulator/AuthUtil.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/Keychain/Keychain.h>
#include <nlohmann/json.hpp>

#include <zlib.h>

#include <atomic>
#include <chrono>
#include <stdlib.h>
#include <thread>

using namespace NoobWarrior;

static Init sInit {};
static NoobWarrior::Core* sCore;
static EmuDb* sEmuDb;

TEST(Core, Init) {
    sCore = new Core(sInit);
    EXPECT_EQ(sCore->Fail(), false) << "noobWarrior failed to initialize. You can assume the test is over now.";
}

TEST(Core, ServerEmulatorHasStableRuntimeIdentity) {
    ASSERT_NE(sCore, nullptr);
    ServerEmulator *emulator = sCore->GetServerEmulator();
    ASSERT_NE(emulator, nullptr);
    const std::string identity = emulator->GetInstanceId();
    EXPECT_FALSE(identity.empty());
    EXPECT_EQ(identity, emulator->GetInstanceId());
}

TEST(Core, ServerEmulatorStoresUserProfilesById) {
    ASSERT_NE(sCore, nullptr);
    ServerEmulator *emulator = sCore->GetServerEmulator();
    ASSERT_NE(emulator, nullptr);

    emulator->ClearUserProfileIdentities();
    emulator->SetUserProfileIdentity(101, "FirstUser", "First Display");
    emulator->SetUserProfileIdentity(202, "SecondUser", "Second Display");

    const std::optional<UserProfileIdentity> first = emulator->GetUserProfileIdentity(101);
    const std::optional<UserProfileIdentity> second = emulator->GetUserProfileIdentity(202);
    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(first->Username, "FirstUser");
    EXPECT_EQ(first->DisplayName, "First Display");
    EXPECT_EQ(second->Username, "SecondUser");
    EXPECT_EQ(second->DisplayName, "Second Display");
    EXPECT_FALSE(emulator->GetUserProfileIdentity(303).has_value());

    emulator->ClearUserProfileIdentities();
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

TEST(Database, AttachHistoricalDataToAsset) {
    SqlDb::Response res = sEmuDb->AttachHistoricalDataToAsset(1, {
        {"Sales", 10},
        {"Favorites", 5},
    });
    EXPECT_EQ(SqlDb::Response::Success, res)
        << "Failed to attach historical data to asset ID 1.";

    // Attaching again should upsert (ON CONFLICT) rather than fail on the primary key.
    res = sEmuDb->AttachHistoricalDataToAsset(1, {{"Sales", 20}});
    EXPECT_EQ(SqlDb::Response::Success, res)
        << "Failed to upsert historical data for asset ID 1.";
}

TEST(Database, DetachHistoricalDataFromAsset) {
    // A non-empty row blanks just the named columns and keeps the record.
    SqlDb::Response res = sEmuDb->DetachHistoricalDataFromAsset(1, {{"Favorites", 0}});
    EXPECT_EQ(SqlDb::Response::Success, res)
        << "Failed to clear a historical column for asset ID 1.";

    // An empty row removes the whole record.
    res = sEmuDb->DetachHistoricalDataFromAsset(1, {});
    EXPECT_EQ(SqlDb::Response::Success, res)
        << "Failed to remove historical data for asset ID 1.";

    // The record is gone, so removing it again reports NotFound.
    res = sEmuDb->DetachHistoricalDataFromAsset(1, {});
    EXPECT_EQ(SqlDb::Response::NotFound, res)
        << "Removing already-removed historical data should report NotFound.";
}

TEST(Database, AttachMicrotransactionDataToAsset) {
    SqlDb::Response res = sEmuDb->AttachMicrotransactionDataToAsset(1, {
        {"CurrencyType", 1},
        {"Price", 25},
    });
    EXPECT_EQ(SqlDb::Response::Success, res)
        << "Failed to attach microtransaction data to asset ID 1.";
}

TEST(Database, AddAndRemoveAssetToBundle) {
    SqlDb::Response res = sEmuDb->AddAssetToBundle(100, 1);
    EXPECT_EQ(SqlDb::Response::Success, res)
        << "Failed to add asset 1 to bundle 100.";

    // Re-linking the same pair is a no-op.
    res = sEmuDb->AddAssetToBundle(100, 1);
    EXPECT_EQ(SqlDb::Response::DidNothing, res)
        << "Re-adding asset 1 to bundle 100 should do nothing.";

    res = sEmuDb->RemoveAssetFromBundle(100, 1);
    EXPECT_EQ(SqlDb::Response::Success, res)
        << "Failed to remove asset 1 from bundle 100.";

    // Removing a link that isn't there reports NotFound.
    res = sEmuDb->RemoveAssetFromBundle(100, 1);
    EXPECT_EQ(SqlDb::Response::NotFound, res)
        << "Removing a non-existent bundle link should report NotFound.";
}

TEST(Database, AddAndRemoveThumbnailFromPlace) {
    SqlDb::Response res = sEmuDb->AddThumbnailToPlace(1, 1);
    EXPECT_EQ(SqlDb::Response::Success, res)
        << "Failed to add a thumbnail to place 1.";

    res = sEmuDb->AddThumbnailToPlace(1, 1);
    EXPECT_EQ(SqlDb::Response::DidNothing, res)
        << "Re-adding the same place thumbnail should do nothing.";

    res = sEmuDb->RemoveThumbnailFromPlace(1, 1);
    EXPECT_EQ(SqlDb::Response::Success, res)
        << "Failed to remove a thumbnail from place 1.";
}

TEST(Database, DetachDataFromAsset) {
    // Asset 1 has data versions 1 and 2 attached above; drop version 1 explicitly.
    SqlDb::Response res = sEmuDb->DetachDataFromAsset(1, 1);
    EXPECT_EQ(SqlDb::Response::Success, res)
        << "Failed to detach data version 1 from asset ID 1.";

    // Version 1 is gone now, so detaching it again reports NotFound.
    res = sEmuDb->DetachDataFromAsset(1, 1);
    EXPECT_EQ(SqlDb::Response::NotFound, res)
        << "Detaching an already-removed version should report NotFound.";
}

TEST(Database, RenderThumbnailForAsset) {
    // Rendering needs the (not-yet-wired) RCCService pipeline, so this reports failure for a real
    // asset and NotFound for one that doesn't exist.
    EXPECT_EQ(SqlDb::Response::Failed, sEmuDb->RenderThumbnailForAsset(1))
        << "RenderThumbnailForAsset should report failure until RCC rendering is implemented.";
    EXPECT_EQ(SqlDb::Response::NotFound, sEmuDb->RenderThumbnailForAsset(999999))
        << "RenderThumbnailForAsset should report NotFound for a missing asset.";
}

TEST(Database, RetrieveAssetDataHashSelectsVersionWithoutLoadingBlob) {
    EmuDb db(":memory:");
    ASSERT_FALSE(db.Fail());
    ASSERT_EQ(SqlDb::Response::Success,
              db.AddItem(ItemType::User, {{"Id", 700}, {"Name", "HashOwner"}}));
    ASSERT_EQ(SqlDb::Response::Success, db.AddItem(ItemType::Asset, {
        {"Id", 701}, {"Name", "Hashed Place"},
        {"Type", static_cast<int>(Roblox::AssetType::Place)}, {"UserId", 700}
    }));
    ASSERT_EQ(SqlDb::Response::Success,
              db.AttachDataToAsset(701, 1, {'o', 'n', 'e'}));
    ASSERT_EQ(SqlDb::Response::Success,
              db.AttachDataToAsset(701, 2, {'t', 'w', 'o'}));

    std::string versionOne;
    std::string versionTwo;
    std::string latest;
    EXPECT_EQ(SqlDb::Response::Success,
              db.RetrieveAssetDataHash(701, 1, &versionOne));
    EXPECT_EQ(SqlDb::Response::Success,
              db.RetrieveAssetDataHash(701, 2, &versionTwo));
    EXPECT_EQ(SqlDb::Response::Success,
              db.RetrieveAssetDataHash(701, 0, &latest));
    EXPECT_EQ(64u, versionOne.size());
    EXPECT_EQ(64u, versionTwo.size());
    EXPECT_NE(versionOne, versionTwo);
    EXPECT_EQ(versionTwo, latest);

    EXPECT_EQ(SqlDb::Response::MissingBlob,
              db.RetrieveAssetDataHash(701, 3, &latest));
    EXPECT_EQ(SqlDb::Response::NotFound,
              db.RetrieveAssetDataHash(999999, 0, &latest));
    EXPECT_EQ(SqlDb::Response::Misuse,
              db.RetrieveAssetDataHash(701, 0, nullptr));
}

TEST(Database, UniversePlaceMapping) {
    // Self-contained DB so this test doesn't depend on the shared sEmuDb's state/order. This covers
    // the lookups behind the /universes/v1/places/{id}/universe, /v1/games and place-details handlers.
    EmuDb db(":memory:");
    ASSERT_EQ(false, db.Fail());

    ASSERT_EQ(SqlDb::Response::Success, db.AddItem(ItemType::User, {
        {"Id", 99}, {"Name", "Hattozo"}
    }));
    ASSERT_EQ(SqlDb::Response::Success, db.AddItem(ItemType::Asset, {
        {"Id", 1818}, {"Name", "My Place"}, {"Type", static_cast<int>(Roblox::AssetType::Place)}, {"UserId", 99}
    }));
    ASSERT_EQ(SqlDb::Response::Success, db.AddItem(ItemType::Asset, {
        {"Id", 2020}, {"Name", "Other Place"}, {"Type", static_cast<int>(Roblox::AssetType::Place)}, {"UserId", 99}
    }));
    // Universe 8 reaches its place both ways (junction row + StartPlaceId); universe 9 only via StartPlaceId.
    ASSERT_EQ(SqlDb::Response::Success, db.AddItem(ItemType::Universe, {
        {"Id", 8}, {"Name", "My Game"}, {"StartPlaceId", 1818}, {"UserId", 99}
    }));
    ASSERT_EQ(SqlDb::Response::Success, db.AddItem(ItemType::Universe, {
        {"Id", 9}, {"Name", "Other Game"}, {"StartPlaceId", 2020}, {"UserId", 99}
    }));
    ASSERT_EQ(true, db.ExecStatement("INSERT INTO UniversePlace (Id, PlaceId) VALUES (8, 1818);"));

    // place -> universe, via the junction table and via StartPlaceId
    auto viaJunction = db.GetUniverseIdForPlace(1818);
    ASSERT_TRUE(viaJunction.has_value());
    EXPECT_EQ(8, viaJunction.value()) << "Place 1818 should resolve to universe 8 (UniversePlace link).";

    auto viaStartPlace = db.GetUniverseIdForPlace(2020);
    ASSERT_TRUE(viaStartPlace.has_value());
    EXPECT_EQ(9, viaStartPlace.value()) << "Place 2020 should resolve to universe 9 (Universe.StartPlaceId).";

    // universe -> root place
    auto rootPlace = db.GetStartPlaceIdForUniverse(8);
    ASSERT_TRUE(rootPlace.has_value());
    EXPECT_EQ(1818, rootPlace.value());
    
    EXPECT_EQ(true, db.GetUniverseVoiceChatEnabled(8).value_or(false));
    ASSERT_TRUE(db.ExecStatement(
        "INSERT INTO UniverseMisc (Id, VoiceChatEnabled) VALUES (8, 0);"));
    EXPECT_EQ(false, db.GetUniverseVoiceChatEnabled(8).value_or(true));

    // names and creator
    EXPECT_EQ("My Game", db.GetItemName(ItemType::Universe, 8).value_or(""));
    EXPECT_EQ("My Place", db.GetItemName(ItemType::Asset, 1818).value_or(""));
    EXPECT_EQ("Hattozo", db.GetItemName(ItemType::User, 99).value_or(""));
    EXPECT_EQ(99, db.GetCreatorUserId(ItemType::Universe, 8).value_or(-1));
    EXPECT_EQ(99, db.GetCreatorUserId(ItemType::Asset, 2020).value_or(-1));

    // misses return nullopt rather than a bogus value
    EXPECT_FALSE(db.GetUniverseIdForPlace(424242).has_value());
    EXPECT_FALSE(db.GetStartPlaceIdForUniverse(424242).has_value());
    EXPECT_FALSE(db.GetUniverseVoiceChatEnabled(424242).has_value());
    EXPECT_FALSE(db.GetItemName(ItemType::Universe, 424242).has_value());
}

TEST(Database, ToolboxAssetSearch) {
    // Backs the toolbox-service search/details and the legacy /IDE/Toolbox endpoints.
    EmuDb db(":memory:");
    ASSERT_EQ(false, db.Fail());

    ASSERT_EQ(SqlDb::Response::Success, db.AddItem(ItemType::User, {{"Id", 99}, {"Name", "Hattozo"}}));
    ASSERT_EQ(SqlDb::Response::Success, db.AddItem(ItemType::Asset, {
        {"Id", 101}, {"Name", "Cool Car"}, {"Description", "vroom"},
        {"Type", static_cast<int>(Roblox::AssetType::Model)}, {"UserId", 99}, {"Created", 1420070400}
    }));
    ASSERT_EQ(SqlDb::Response::Success, db.AddItem(ItemType::Asset, {
        {"Id", 102}, {"Name", "Cool Sound"}, {"Type", static_cast<int>(Roblox::AssetType::Audio)}, {"UserId", 99}
    }));
    ASSERT_EQ(SqlDb::Response::Success, db.AddItem(ItemType::Asset, {
        {"Id", 103}, {"Name", "Cooler Car"}, {"Type", static_cast<int>(Roblox::AssetType::Model)}, {"UserId", 99}
    }));

    // type filter: only the two Models, newest id first
    std::vector<int64_t> models = db.SearchAssetIds(Roblox::AssetType::Model, "", 30, 0);
    ASSERT_EQ(2u, models.size());
    EXPECT_EQ(103, models[0]);
    EXPECT_EQ(101, models[1]);

    // keyword filter narrows within the type
    std::vector<int64_t> cooler = db.SearchAssetIds(Roblox::AssetType::Model, "Cooler", 30, 0);
    ASSERT_EQ(1u, cooler.size());
    EXPECT_EQ(103, cooler[0]);

    // a different type doesn't bleed in
    std::vector<int64_t> audio = db.SearchAssetIds(Roblox::AssetType::Audio, "", 30, 0);
    ASSERT_EQ(1u, audio.size());
    EXPECT_EQ(102, audio[0]);

    // limit is honored
    EXPECT_EQ(1u, db.SearchAssetIds(Roblox::AssetType::Model, "", 1, 0).size());

    // summary fields
    auto summary = db.GetAssetSummary(101);
    ASSERT_TRUE(summary.has_value());
    EXPECT_EQ("Cool Car", summary->Name);
    EXPECT_EQ("vroom", summary->Description);
    EXPECT_EQ(static_cast<int>(Roblox::AssetType::Model), summary->Type);
    ASSERT_TRUE(summary->UserId.has_value());
    EXPECT_EQ(99, summary->UserId.value());
    EXPECT_EQ(1420070400, summary->Created);

    EXPECT_FALSE(db.GetAssetSummary(999999).has_value());
}

TEST(Database, DeleteAsset) {
    // Counts rows from a single-column COUNT(*) query against the shared fixture database.
    auto count = [](const std::string &sql) -> int64_t {
        Statement stmt = sEmuDb->PrepareStatement(sql);
        if (stmt.Fail() || stmt.Step() != SQLITE_ROW) return -1;
        return stmt.GetInt64FromColumnIndex(0);
    };

    // Earlier tests attached data to asset 1, so it owns AssetData version(s) and a blob. Capture
    // the blob hash before deleting so we can confirm it gets garbage-collected too.
    Statement hashStmt = sEmuDb->PrepareStatement("SELECT DataHash FROM AssetData WHERE Id = 1 LIMIT 1;");
    ASSERT_EQ(SQLITE_ROW, hashStmt.Step()) << "Asset 1 should have AssetData before deletion.";
    std::string blobHash = hashStmt.GetStringFromColumnIndex(0);
    EXPECT_GT(count("SELECT COUNT(*) FROM AssetData WHERE Id = 1;"), 0);
    EXPECT_EQ(1, count("SELECT COUNT(*) FROM BlobStorage WHERE Hash = '" + blobHash + "';"));

    SqlDb::Response res = sEmuDb->DeleteItem(ItemType::Asset, 1);
    EXPECT_EQ(SqlDb::Response::Success, res)
        << "Failed to delete the data for asset ID 1. Check the quality of the EmuDb::DeleteItem() function.";

    // The parent row and its dependent AssetData rows must be gone, and the now-unreferenced blob
    // must have been garbage-collected.
    EXPECT_EQ(0, count("SELECT COUNT(*) FROM Asset WHERE Id = 1;"))
        << "DeleteItem left the parent Asset row behind.";
    EXPECT_EQ(0, count("SELECT COUNT(*) FROM AssetData WHERE Id = 1;"))
        << "DeleteItem did not cascade to the AssetData rows.";
    EXPECT_EQ(0, count("SELECT COUNT(*) FROM BlobStorage WHERE Hash = '" + blobHash + "';"))
        << "DeleteItem did not garbage-collect the orphaned blob.";
}

TEST(Database, Close) {
    delete sEmuDb;
}

TEST(Auth, PasswordHashRoundTrip) {
    std::vector<unsigned char> salt(AuthUtil::kSaltLength, 0xAB);
    std::string hash = AuthUtil::HashPassword("correcthorse", salt);
    ASSERT_FALSE(hash.empty()) << "HashPassword returned empty; Argon2id derivation failed.";

    std::string saltHex = AuthUtil::ToHex(salt.data(), salt.size());
    EXPECT_TRUE(AuthUtil::VerifyPassword("correcthorse", saltHex, hash))
        << "VerifyPassword rejected the correct password.";
    EXPECT_FALSE(AuthUtil::VerifyPassword("wrongpassword", saltHex, hash))
        << "VerifyPassword accepted an incorrect password.";
}

TEST(Auth, GuestTicketRoundTrip) {
    AuthUtil::SessionUser guest = AuthUtil::MakeGuestUser();
    EXPECT_TRUE(guest.isGuest);
    EXPECT_LT(guest.id, 0) << "Guests must have a negative id so they never collide with real accounts.";

    std::string ticket = AuthUtil::EncodeGuestTicket(guest);
    auto decoded = AuthUtil::DecodeGuestTicket(ticket);
    ASSERT_TRUE(decoded.has_value()) << "A guest ticket should decode back to a guest.";
    EXPECT_EQ(guest.id, decoded->id);
    EXPECT_EQ(guest.displayName, decoded->displayName);
    EXPECT_TRUE(decoded->isGuest);

    // A real (non-guest) ticket must not be mistaken for a guest.
    EXPECT_FALSE(AuthUtil::DecodeGuestTicket("deadbeef").has_value());
}

TEST(Auth, Base64UrlRoundTrip) {
    for (const std::string &s : {std::string(""), std::string("a"), std::string("ab"), std::string("abc"),
                                 std::string("alice@masterA.example"), std::string("join\nalice@a\nb\nnonce"),
                                 std::string("\x00\x01\x02\xff", 4)}) {
        std::string encoded = AuthUtil::Base64UrlEncode(s);
        EXPECT_EQ(encoded.find('='), std::string::npos) << "base64url must not emit padding.";
        auto decoded = AuthUtil::Base64UrlDecode(encoded);
        ASSERT_TRUE(decoded.has_value());
        EXPECT_EQ(*decoded, s);
    }
    EXPECT_FALSE(AuthUtil::Base64UrlDecode("has spaces").has_value()) << "invalid chars must fail to decode";
}

TEST(Auth, FederatedTicketRoundTrip) {
    AuthUtil::SessionUser user;
    user.id = 1000012345;                 // OnlineUserId range is [1e9, 2^53)
    user.name = "alice@masterA.example";
    user.displayName = "alice";
    user.isFederated = true;

    std::string ticket = AuthUtil::EncodeFederatedTicket(user);
    auto decoded = AuthUtil::DecodeFederatedTicket(ticket);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->id, user.id);
    EXPECT_EQ(decoded->name, user.name);
    EXPECT_EQ(decoded->displayName, user.displayName);
    EXPECT_TRUE(decoded->isFederated);
    EXPECT_FALSE(decoded->isGuest);

    // A federated ticket carries a positive id, so it must never decode as a guest, and vice-versa.
    EXPECT_FALSE(AuthUtil::DecodeGuestTicket(ticket).has_value());
    EXPECT_FALSE(AuthUtil::DecodeFederatedTicket(AuthUtil::EncodeGuestTicket(AuthUtil::MakeGuestUser())).has_value());
    EXPECT_FALSE(AuthUtil::DecodeFederatedTicket("deadbeef").has_value());
    EXPECT_FALSE(AuthUtil::DecodeFederatedTicket("fed:notanumber:aa:bb").has_value());
}

TEST(Auth, ResolveSessionUser) {
    EmuDb db(":memory:");
    ASSERT_FALSE(db.Fail());
    ASSERT_EQ(SqlDb::Response::Success, db.AddItem(ItemType::User, {
        {"Id", 1000}, {"Name", "alice"}, {"DisplayName", "Alice"}
    }));
    ASSERT_TRUE(db.ExecStatement("INSERT INTO LoginSession (Token, UserId) VALUES ('tok-alice', 1000);"));

    auto user = AuthUtil::ResolveSessionUser(&db, "tok-alice");
    ASSERT_TRUE(user.has_value()) << "A valid session token should resolve to its user.";
    EXPECT_EQ(1000, user->id);
    EXPECT_EQ("alice", user->name);
    EXPECT_EQ("Alice", user->displayName);

    EXPECT_FALSE(AuthUtil::ResolveSessionUser(&db, "nope").has_value())
        << "An unknown token must not resolve to a user.";
    EXPECT_FALSE(AuthUtil::ResolveSessionUser(&db, "").has_value())
        << "An empty token must not resolve to a user.";
}

TEST(Auth, AuthTicketMintRedeemSingleUse) {
    EmuDb db(":memory:");
    ASSERT_FALSE(db.Fail());
    ASSERT_EQ(SqlDb::Response::Success, db.AddItem(ItemType::User, {
        {"Id", 2000}, {"Name", "bob"}, {"DisplayName", "Bob"}
    }));

    std::string ticket = AuthUtil::MintAuthTicket(&db, 2000, 1818);
    ASSERT_FALSE(ticket.empty()) << "MintAuthTicket should return a ticket string.";

    auto first = AuthUtil::RedeemAuthTicket(&db, ticket, 120);
    ASSERT_TRUE(first.has_value()) << "A fresh ticket should redeem successfully.";
    EXPECT_EQ(2000, first->id);
    EXPECT_EQ("bob", first->name);

    // Single-use: a second redeem of the same ticket must fail.
    EXPECT_FALSE(AuthUtil::RedeemAuthTicket(&db, ticket, 120).has_value())
        << "A ticket must not be redeemable twice.";
}

TEST(Auth, AuthTicketExpiry) {
    EmuDb db(":memory:");
    ASSERT_FALSE(db.Fail());
    ASSERT_EQ(SqlDb::Response::Success, db.AddItem(ItemType::User, {
        {"Id", 3000}, {"Name", "carol"}, {"DisplayName", "Carol"}
    }));

    std::string ticket = AuthUtil::MintAuthTicket(&db, 3000, 0);
    ASSERT_FALSE(ticket.empty());

    // A TTL of 0 means the ticket is already expired (age is never < 0), so redeem must fail.
    EXPECT_FALSE(AuthUtil::RedeemAuthTicket(&db, ticket, 0).has_value())
        << "An expired ticket (ttl=0) must not redeem.";
    // ...and having failed the age check, it is still unredeemed, so a valid TTL still works.
    EXPECT_TRUE(AuthUtil::RedeemAuthTicket(&db, ticket, 120).has_value())
        << "A ticket that only failed the age check should still be redeemable within TTL.";
}

TEST(Auth, CreateLoginSession) {
    EmuDb db(":memory:");
    ASSERT_FALSE(db.Fail());
    ASSERT_EQ(SqlDb::Response::Success, db.AddItem(ItemType::User, {
        {"Id", 4000}, {"Name", "dave"}, {"DisplayName", "Dave"}
    }));

    std::string token = AuthUtil::CreateLoginSession(&db, 4000, "127.0.0.1", "UnitTest");
    ASSERT_FALSE(token.empty()) << "CreateLoginSession should return a session token.";

    auto user = AuthUtil::ResolveSessionUser(&db, token);
    ASSERT_TRUE(user.has_value()) << "The created session must resolve back to its user.";
    EXPECT_EQ(4000, user->id);
    EXPECT_EQ("dave", user->name);
}

TEST(Auth, SessionTtlAndReap) {
    EmuDb db(":memory:");
    ASSERT_FALSE(db.Fail());
    ASSERT_EQ(SqlDb::Response::Success, db.AddItem(ItemType::User, {
        {"Id", 5000}, {"Name", "frank"}, {"DisplayName", "Frank"}
    }));

    std::string token = AuthUtil::CreateLoginSession(&db, 5000, "127.0.0.1", "UnitTest");
    ASSERT_FALSE(token.empty());

    // Backdate the session so it looks idle for 100s.
    Statement age = db.PrepareStatement("UPDATE LoginSession SET LastUsedTimestamp = unixepoch() - 100 WHERE Token = ?;");
    age.Bind(1, token);
    ASSERT_EQ(SQLITE_DONE, age.Step());

    // A 50s TTL treats a 100s-idle session as expired; TTL 0 (disabled) still resolves it.
    EXPECT_FALSE(AuthUtil::ResolveSessionUser(&db, token, 50).has_value()) << "idle > TTL must not resolve.";
    EXPECT_EQ(0, AuthUtil::ReapExpiredSessions(&db, 0)) << "ttl 0 reaps nothing.";
    EXPECT_EQ(1, AuthUtil::ReapExpiredSessions(&db, 50)) << "the expired session should be reaped.";
    EXPECT_FALSE(AuthUtil::ResolveSessionUser(&db, token, 0).has_value()) << "reaped session is gone.";

    // A fresh session resolves within the TTL and isn't reaped.
    std::string fresh = AuthUtil::CreateLoginSession(&db, 5000, "127.0.0.1", "UnitTest");
    ASSERT_FALSE(fresh.empty());
    EXPECT_TRUE(AuthUtil::ResolveSessionUser(&db, fresh, 50).has_value());
    EXPECT_EQ(0, AuthUtil::ReapExpiredSessions(&db, 50));
}

TEST(Keychain, AccountJsonRoundTrip) {
    // url + display_name must survive a serialize/deserialize round-trip (needed for master/emu accounts).
    Account acc {};
    acc.Id = 12345;
    acc.Name = "alice@a";
    acc.DisplayName = "Alice";
    acc.Token = "sess-token";
    acc.Url = "http://a:8090";
    acc.ExpireTimestamp = -1;

    nlohmann::json j = Keychain::AccStructToJson(acc);
    Account back = Keychain::AccJsonToStruct(j);
    EXPECT_EQ(12345, back.Id);
    EXPECT_EQ("alice@a", back.Name);
    EXPECT_EQ("Alice", back.DisplayName);
    EXPECT_EQ("sess-token", back.Token);
    EXPECT_EQ("http://a:8090", back.Url);
    EXPECT_EQ(-1, back.ExpireTimestamp);

    // A legacy record predating url/display_name decodes with empty defaults instead of throwing.
    nlohmann::json legacy = {{"id", 7}, {"name", "bob"}, {"token", "t"}, {"expire_timestamp", -1}};
    Account l = Keychain::AccJsonToStruct(legacy);
    EXPECT_EQ(7, l.Id);
    EXPECT_EQ("bob", l.Name);
    EXPECT_TRUE(l.Url.empty());
    EXPECT_TRUE(l.DisplayName.empty());
}

TEST(Auth, CreateAndDeleteLocalAccount) {
    EmuDb db(":memory:");
    ASSERT_FALSE(db.Fail());

    EXPECT_FALSE(AuthUtil::LocalAccountExists(&db, "erin"));
    auto id = AuthUtil::CreateLocalAccount(&db, "erin", "hunter2", "Erin");
    ASSERT_TRUE(id.has_value()) << "CreateLocalAccount should return the new user's id.";
    EXPECT_GT(*id, 0);
    EXPECT_TRUE(AuthUtil::LocalAccountExists(&db, "erin"));

    // The account shows up in the listing with its display name.
    auto accounts = AuthUtil::ListLocalAccounts(&db);
    ASSERT_EQ(1u, accounts.size());
    EXPECT_EQ(*id, accounts[0].id);
    EXPECT_EQ("erin", accounts[0].name);
    EXPECT_EQ("Erin", accounts[0].displayName);

    // The password was hashed (Argon2id) and verifies.
    Statement stmt = db.PrepareStatement("SELECT PasswordHash, PasswordSalt FROM User WHERE Id = ?;");
    stmt.Bind(1, *id);
    ASSERT_EQ(SQLITE_ROW, stmt.Step());
    EXPECT_TRUE(AuthUtil::VerifyPassword("hunter2", stmt.GetStringFromColumnIndex(1), stmt.GetStringFromColumnIndex(0)));

    // A duplicate username (case-insensitive) is refused.
    EXPECT_FALSE(AuthUtil::CreateLocalAccount(&db, "ERIN", "x", "").has_value());

    // Deleting removes it from the listing.
    EXPECT_TRUE(AuthUtil::DeleteLocalAccount(&db, *id));
    EXPECT_FALSE(AuthUtil::LocalAccountExists(&db, "erin"));
    EXPECT_TRUE(AuthUtil::ListLocalAccounts(&db).empty());
}

TEST(Auth, Ed25519SignVerify) {
    std::string priv, pub;
    ASSERT_TRUE(AuthUtil::GenerateEd25519(priv, pub));
    EXPECT_EQ(priv.size(), 64u);   // 32 bytes hex
    EXPECT_EQ(pub.size(), 64u);

    std::string msg = "join\nalice@a\nb\nnonce123";
    std::string sig = AuthUtil::Ed25519Sign(priv, msg);
    ASSERT_FALSE(sig.empty());
    EXPECT_EQ(sig.size(), 128u);   // 64 bytes hex

    EXPECT_TRUE(AuthUtil::Ed25519Verify(pub, msg, sig)) << "a valid signature must verify.";

    // Tampered message, tampered signature, and a different key all fail.
    EXPECT_FALSE(AuthUtil::Ed25519Verify(pub, msg + "x", sig)) << "tampered message must not verify.";
    std::string badSig = sig;
    badSig[0] = (badSig[0] == 'a') ? 'b' : 'a';
    EXPECT_FALSE(AuthUtil::Ed25519Verify(pub, msg, badSig)) << "tampered signature must not verify.";

    std::string priv2, pub2;
    ASSERT_TRUE(AuthUtil::GenerateEd25519(priv2, pub2));
    EXPECT_FALSE(AuthUtil::Ed25519Verify(pub2, msg, sig)) << "another key's signature must not verify.";

    // Malformed inputs are rejected, not crashed on.
    EXPECT_FALSE(AuthUtil::Ed25519Verify("notahexkey", msg, sig));
    EXPECT_TRUE(AuthUtil::Ed25519Sign("short", msg).empty());
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
