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
// File: DatabaseManager.h
// Started by: Hattozo
// Started on: 3/17/2025
// Description:
#pragma once
#include <NoobWarrior/EmuDb/EmuDb.h>
#include <NoobWarrior/EmuDb/Repository/Repository.h>

#include <filesystem>
#include <vector>

namespace NoobWarrior {
class Core;
class EmuDbManager {
public:
    EmuDbManager(Core *core);

    void MountDatabases();

    // NOTE: THIS DELETES ALL THE MOUNTED DATABASES FROM MEMORY!!!!
    void UnmountDatabases();

    // CALL MOUNTDATABASES() BEFORE WASTING YOUR TIME ON THIS FUNCTION!!!
    SqlDb::Response MountMasterDbIfNotAlreadyMounted();

    SqlDb::FailReason Mount(const std::string &fileName, unsigned int priority);
    bool Mount(EmuDb* database, unsigned int priority);
    bool Unmount(EmuDb* database);

    EmuDb *GetMasterDatabase();
    std::vector<EmuDb*> GetMountedDatabases();

    bool GetUserFromToken(User *user, const std::string &token);

    SqlDb::Response RetrieveAssetData(int64_t id, int version, std::vector<unsigned char> *dataOutput, std::string *hashOutput = nullptr);
private:
    Core* mCore;
    std::vector<EmuDb*> mMountedDatabases;
};
}