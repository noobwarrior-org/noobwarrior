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

using namespace NoobWarrior;

static Init sInit {};
static NoobWarrior::Core* sCore;
static EmuDb* sEmuDb;

TEST(NoobWarrior, Init) {
    sCore = new Core(sInit);
    EXPECT_EQ(sCore->Fail(), false);
}

TEST(NoobWarrior, OpenTemporaryDatabase) {
    EmuDb* db = new EmuDb(":memory:");
    EXPECT_EQ(db->Fail(), false);
    delete db;
}

TEST(NoobWarrior, OpenPermanentDatabase) {
    sEmuDb = new EmuDb(":memory:"); // Contrary to what the test title says, this database is still residing in memory
    EXPECT_EQ(sEmuDb->Fail(), false);
}

TEST(NoobWarrior, DatabaseAddAsset) {
    SqlDb::Response res = sEmuDb->AddItem(ItemType::Asset, {
        {"Id", 1},
        {"Name", "Test"},
        {"Description", "Test Description"},
        {"Type", static_cast<int>(Roblox::AssetType::Model)}
    });
    EXPECT_EQ(static_cast<int>(res), static_cast<int>(SqlDb::Response::Success));
}

TEST(NoobWarrior, DatabaseDeleteAsset) {
    SqlDb::Response res = sEmuDb->DeleteItem(ItemType::Asset, 1);
    EXPECT_EQ(static_cast<int>(res), static_cast<int>(SqlDb::Response::Success));
}

int main(int argc, char** argv) {
    sInit.ArgCount = argc;
    sInit.ArgVec = argv;
    
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
