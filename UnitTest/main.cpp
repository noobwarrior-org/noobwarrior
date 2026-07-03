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

    // names and creator
    EXPECT_EQ("My Game", db.GetItemName(ItemType::Universe, 8).value_or(""));
    EXPECT_EQ("My Place", db.GetItemName(ItemType::Asset, 1818).value_or(""));
    EXPECT_EQ("Hattozo", db.GetItemName(ItemType::User, 99).value_or(""));
    EXPECT_EQ(99, db.GetCreatorUserId(ItemType::Universe, 8).value_or(-1));
    EXPECT_EQ(99, db.GetCreatorUserId(ItemType::Asset, 2020).value_or(-1));

    // misses return nullopt rather than a bogus value
    EXPECT_FALSE(db.GetUniverseIdForPlace(424242).has_value());
    EXPECT_FALSE(db.GetStartPlaceIdForUniverse(424242).has_value());
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
