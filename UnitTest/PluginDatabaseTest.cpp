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
// File: PluginDatabaseTest.cpp
// Started by: Hattozo
// Started on: 8/27/2026
// Description: Read-only mounts, the Mutable flag, and EmuDbManager mount ownership. These back the
// plugin-provided database feature, whose whole promise is that a plugin's own file is never written
// to and never leaks into the user's own list of mounted databases.
#include <NoobWarrior/EmuDb/EmuDb.h>
#include <NoobWarrior/EmuDb/EmuDbManager.h>

#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

using namespace NoobWarrior;

namespace {
// Each test gets its own file; EmuDb needs a real path, since a read-only open is meaningless for
// an in-memory database.
class TempDb {
public:
    explicit TempDb(const char *name) {
        const ::testing::TestInfo *info = ::testing::UnitTest::GetInstance()->current_test_info();
        mPath = std::filesystem::temp_directory_path() /
            ("nw_test_" + std::string(info != nullptr ? info->name() : "unknown") + "_" + name + ".nwdb");
        std::filesystem::remove(mPath);
    }
    ~TempDb() {
        std::error_code ec;
        std::filesystem::remove(mPath, ec);
    }
    const std::filesystem::path &Path() const { return mPath; }
    std::string Str() const { return mPath.string(); }

    std::vector<unsigned char> ReadBytes() const {
        std::ifstream in(mPath, std::ios::binary);
        return std::vector<unsigned char>(std::istreambuf_iterator<char>(in),
                                          std::istreambuf_iterator<char>());
    }
private:
    std::filesystem::path mPath;
};

// Creates the file and closes it again, so the next open sees a finished database.
void CreateDb(const TempDb &temp, bool mutableFlag = true) {
    EmuDb db(temp.Str(), true);
    ASSERT_FALSE(db.Fail());
    db.SetTitle("Fixture");
    db.SetMutable(mutableFlag);
}
} // namespace

TEST(PluginDatabase, MutableDefaultsToTrue) {
    EmuDb db(":memory:");
    ASSERT_FALSE(db.Fail());
    EXPECT_TRUE(db.IsMutable());
    EXPECT_TRUE(db.AllowsRuntimeWrites());
}

TEST(PluginDatabase, MutableRoundTrips) {
    EmuDb db(":memory:");
    ASSERT_FALSE(db.Fail());

    EXPECT_EQ(SqlDb::Response::Success, db.SetMutable(false));
    EXPECT_FALSE(db.IsMutable());
    EXPECT_FALSE(db.AllowsRuntimeWrites());

    EXPECT_EQ(SqlDb::Response::Success, db.SetMutable(true));
    EXPECT_TRUE(db.IsMutable());
    EXPECT_TRUE(db.AllowsRuntimeWrites());
}

TEST(PluginDatabase, ProbeIsMutableReadsTheFlagWithoutOpeningTheDatabase) {
    TempDb temp("probe");
    CreateDb(temp, /*mutableFlag=*/false);
    EXPECT_FALSE(EmuDb::ProbeIsMutable(temp.Path()));

    CreateDb(temp, /*mutableFlag=*/true);
    EXPECT_TRUE(EmuDb::ProbeIsMutable(temp.Path()));
}

TEST(PluginDatabase, ProbeIsMutablePermitsWhatItCannotRead) {
    // A missing file has no opinion, and the caller's real open is what should report the failure.
    EXPECT_TRUE(EmuDb::ProbeIsMutable(
        std::filesystem::temp_directory_path() / "nw_test_definitely_not_here.nwdb"));
}

TEST(PluginDatabase, ReadOnlyOpenRefusesToCreateAMissingFile) {
    TempDb temp("missing");
    EmuDb db(temp.Str(), true, SqlDb::OpenMode::ReadOnly);
    EXPECT_TRUE(db.Fail());
    EXPECT_EQ(SqlDb::FailReason::CantOpen, db.GetFailReason());
    EXPECT_FALSE(std::filesystem::exists(temp.Path()))
        << "a read-only open must never bring the file into existence";
}

TEST(PluginDatabase, ReadOnlyOpenLeavesTheFileByteIdentical) {
    TempDb temp("untouched");
    CreateDb(temp);
    const std::vector<unsigned char> before = temp.ReadBytes();
    ASSERT_FALSE(before.empty());

    {
        EmuDb db(temp.Str(), true, SqlDb::OpenMode::ReadOnly);
        ASSERT_FALSE(db.Fail()) << "reason " << static_cast<int>(db.GetFailReason());
        EXPECT_TRUE(db.IsReadOnly());
        EXPECT_FALSE(db.AllowsRuntimeWrites());

        // The write is refused by SQLite rather than silently dropped.
        EXPECT_NE(SqlDb::Response::Success, db.SetTitle("Should Not Stick"));
    }

    EXPECT_EQ(before, temp.ReadBytes());

    // And no journal or -shm sidecar was left beside it.
    EXPECT_FALSE(std::filesystem::exists(temp.Path().string() + "-journal"));
    EXPECT_FALSE(std::filesystem::exists(temp.Path().string() + "-shm"));
    EXPECT_FALSE(std::filesystem::exists(temp.Path().string() + "-wal"));

    EmuDb reopened(temp.Str(), true);
    ASSERT_FALSE(reopened.Fail());
    EXPECT_EQ("Fixture", reopened.GetTitle());
}

TEST(PluginDatabase, MountInfoDistinguishesUserAndPluginMounts) {
    TempDb userDb("user");
    TempDb pluginDb("plugin");
    CreateDb(userDb);
    CreateDb(pluginDb);

    EmuDbManager manager(nullptr);
    ASSERT_EQ(SqlDb::FailReason::None,
              manager.MountOwned(userDb.Path(), 0, EmuDbManager::MountInfo {}, SqlDb::OpenMode::ReadWrite));

    EmuDbManager::MountInfo pluginInfo;
    pluginInfo.OwnerPluginId = "mygame@example.org";
    pluginInfo.SourceUrl = "plugin://mygame@example.org/databases/content.nwdb";
    pluginInfo.Locked = true;
    ASSERT_EQ(SqlDb::FailReason::None,
              manager.MountOwned(pluginDb.Path(), 1, pluginInfo, SqlDb::OpenMode::ReadOnly));

    ASSERT_EQ(2u, manager.GetMountedDatabases().size());
    EmuDb *mountedUser = manager.GetMountedDatabases().at(0);
    EmuDb *mountedPlugin = manager.GetMountedDatabases().at(1);

    // The master database is index 0, so the plugin must not have displaced it.
    EXPECT_EQ(mountedUser, manager.GetMasterDatabase());

    EXPECT_FALSE(manager.IsLocked(mountedUser));
    EXPECT_TRUE(manager.IsLocked(mountedPlugin));
    EXPECT_TRUE(mountedPlugin->IsReadOnly());

    // Only the user's mount may be written back to databases.mounted.
    ASSERT_EQ(1u, manager.GetUserMountedDatabases().size());
    EXPECT_EQ(mountedUser, manager.GetUserMountedDatabases().at(0));

    EXPECT_EQ(mountedPlugin, manager.GetDbFromSourceUrl(pluginInfo.SourceUrl));
    EXPECT_EQ(nullptr, manager.GetDbFromSourceUrl("plugin://nobody@example.org/x.nwdb"));

    manager.UnmountDatabases();
}

// A database a plugin only offers is not mounted, so the Database dialog has to describe it by
// reading the file directly. The probe must work without a full open and must not touch the file.
TEST(PluginDatabase, ProbeReadsTitleAndIconWithoutOpeningTheDatabase) {
    TempDb temp("offered");
    const std::vector<unsigned char> icon = {0xAB, 0xCD, 0xEF, 0x01, 0x02};
    {
        EmuDb db(temp.Str(), true);
        ASSERT_FALSE(db.Fail());
        ASSERT_EQ(SqlDb::Response::Success, db.SetTitle("Example Content"));
        ASSERT_EQ(SqlDb::Response::Success, db.SetIcon(icon));
    }
    const std::vector<unsigned char> before = temp.ReadBytes();

    EXPECT_EQ("Example Content", EmuDb::ProbeTitle(temp.Path()));
    EXPECT_EQ(icon, EmuDb::ProbeIcon(temp.Path()));
    EXPECT_EQ(before, temp.ReadBytes()) << "probing must not modify the file";

    // Works the same for a database marked immutable, which is the common case for shipped content.
    {
        EmuDb db(temp.Str(), true);
        ASSERT_FALSE(db.Fail());
        ASSERT_EQ(SqlDb::Response::Success, db.SetMutable(false));
    }
    EXPECT_EQ("Example Content", EmuDb::ProbeTitle(temp.Path()));
    EXPECT_EQ(icon, EmuDb::ProbeIcon(temp.Path()));

    // A file that is not there yields empties rather than failing.
    const std::filesystem::path missing =
        std::filesystem::temp_directory_path() / "nw_test_no_such_offer.nwdb";
    EXPECT_TRUE(EmuDb::ProbeTitle(missing).empty());
    EXPECT_TRUE(EmuDb::ProbeIcon(missing).empty());
}

// A read-only database never gets the default icon written into it, so anything displaying one has
// to fall back to the same image itself.
TEST(PluginDatabase, DefaultIconIsUsableWithoutWritingToTheDatabase) {
    const std::vector<unsigned char> icon = EmuDb::GetDefaultIconData();
    ASSERT_GT(icon.size(), 24u);

    const unsigned char png[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    EXPECT_EQ(0, std::memcmp(icon.data(), png, sizeof(png)));

    // IHDR width/height are big-endian at offsets 16 and 20.
    auto be32 = [&icon](std::size_t at) {
        return (static_cast<unsigned>(icon[at]) << 24) | (static_cast<unsigned>(icon[at + 1]) << 16) |
               (static_cast<unsigned>(icon[at + 2]) << 8) | static_cast<unsigned>(icon[at + 3]);
    };
    EXPECT_EQ(256u, be32(16));
    EXPECT_EQ(256u, be32(20)) << "the fallback must not be the 16x16 silk placeholder";

    TempDb temp("noicon");
    CreateDb(temp);
    {
        EmuDb db(temp.Str(), true);
        ASSERT_FALSE(db.Fail());
        ASSERT_EQ(SqlDb::Response::Success, db.SetMetaKeyValue("Icon", ""));
    }
    const std::vector<unsigned char> before = temp.ReadBytes();

    {
        EmuDb readOnly(temp.Str(), true, SqlDb::OpenMode::ReadOnly);
        ASSERT_FALSE(readOnly.Fail());
        EXPECT_TRUE(readOnly.GetIcon().empty()) << "opening read-only must not fill in the icon";
    }
    EXPECT_EQ(before, temp.ReadBytes());
}

TEST(PluginDatabase, UnmountingAPluginLeavesTheUsersMountsAlone) {
    TempDb userDb("keep");
    TempDb pluginDb("drop");
    CreateDb(userDb);
    CreateDb(pluginDb);

    EmuDbManager manager(nullptr);
    manager.MountOwned(userDb.Path(), 0, EmuDbManager::MountInfo {}, SqlDb::OpenMode::ReadWrite);

    EmuDbManager::MountInfo pluginInfo;
    pluginInfo.OwnerPluginId = "mygame@example.org";
    pluginInfo.SourceUrl = "plugin://mygame@example.org/databases/content.nwdb";
    manager.MountOwned(pluginDb.Path(), 1, pluginInfo, SqlDb::OpenMode::ReadOnly);

    manager.UnmountDatabasesForPlugin("mygame@example.org");

    ASSERT_EQ(1u, manager.GetMountedDatabases().size());
    EXPECT_EQ(nullptr, manager.GetDbFromSourceUrl(pluginInfo.SourceUrl));
    EXPECT_EQ(nullptr, manager.GetMountInfo(nullptr));

    manager.UnmountDatabases();
}

TEST(PluginDatabase, RuntimeWritesFallBackToTheMasterDatabase) {
    TempDb masterDb("master");
    TempDb contentDb("content");
    CreateDb(masterDb);
    CreateDb(contentDb);

    // Put an asset only in the read-only content database.
    constexpr int64_t kAssetId = 4242;
    {
        EmuDb content(contentDb.Str(), true);
        ASSERT_FALSE(content.Fail());
        SqlRow row;
        row.push_back({"Id", kAssetId});
        row.push_back({"Type", 9});
        row.push_back({"Name", std::string("Shipped Place")});
        ASSERT_EQ(SqlDb::Response::Success, content.AddItem(ItemType::Asset, row));
    }

    EmuDbManager manager(nullptr);
    manager.MountOwned(masterDb.Path(), 0, EmuDbManager::MountInfo {}, SqlDb::OpenMode::ReadWrite);

    EmuDbManager::MountInfo pluginInfo;
    pluginInfo.OwnerPluginId = "mygame@example.org";
    pluginInfo.SourceUrl = "plugin://mygame@example.org/databases/content.nwdb";
    manager.MountOwned(contentDb.Path(), 1, pluginInfo, SqlDb::OpenMode::ReadOnly);

    EmuDb *master = manager.GetMountedDatabases().at(0);
    EmuDb *content = manager.GetMountedDatabases().at(1);

    // Reads still resolve to the database that actually owns the asset.
    EXPECT_EQ(content, manager.GetFirstDbWhereItemExists(ItemType::Asset, kAssetId));
    // Writes do not: they go to the master, which outranks it and therefore shadows it.
    EXPECT_EQ(master, manager.GetWritableDbForItem(ItemType::Asset, kAssetId));

    manager.UnmountDatabases();
}

// Asset Grab Mode has two writers: AssetHandler writes through the mounted connection, and
// AssetEnricher opens its own private one on a background thread. The second is why Mutable has to
// be a property of the file rather than of a particular mount -- a read-only mount does not
// constrain a connection opened somewhere else.
TEST(PluginDatabase, ASecondConnectionStillHonorsMutable) {
    TempDb temp("grabtarget");
    CreateDb(temp, /*mutableFlag=*/false);

    // What the mount sees.
    EmuDbManager manager(nullptr);
    ASSERT_EQ(SqlDb::FailReason::None,
              manager.MountOwned(temp.Path(), 0, EmuDbManager::MountInfo {}, SqlDb::OpenMode::ReadOnly));
    EXPECT_FALSE(manager.GetMountedDatabases().at(0)->AllowsRuntimeWrites());

    // What a private read-write connection would see if it did not check first. Opening one is
    // entirely possible, which is exactly why the enricher probes before it does.
    EXPECT_FALSE(EmuDb::ProbeIsMutable(temp.Path()));

    manager.UnmountDatabases();
}

TEST(PluginDatabase, MutableOffMakesRuntimeWritesFallBackToo) {
    TempDb masterDb("mmaster");
    TempDb frozenDb("frozen");
    CreateDb(masterDb);
    CreateDb(frozenDb, /*mutableFlag=*/false);

    EmuDbManager manager(nullptr);
    manager.MountOwned(masterDb.Path(), 0, EmuDbManager::MountInfo {}, SqlDb::OpenMode::ReadWrite);
    // Mounted read-write, but the database itself says it is not to be modified at runtime.
    manager.MountOwned(frozenDb.Path(), 1, EmuDbManager::MountInfo {}, SqlDb::OpenMode::ReadWrite);

    EmuDb *master = manager.GetMountedDatabases().at(0);
    EmuDb *frozen = manager.GetMountedDatabases().at(1);
    EXPECT_FALSE(frozen->IsReadOnly());
    EXPECT_FALSE(frozen->AllowsRuntimeWrites());

    constexpr int64_t kUniverseId = 7777;
    EXPECT_EQ(master, manager.GetWritableDbForItem(ItemType::Universe, kUniverseId));

    manager.UnmountDatabases();
}
