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
// File: DatabaseManager.cpp
// Started by: Hattozo
// Started on: 3/17/2025
// Description: Loads in multiple databases with different priorities over one another.
// This is used in situations where you want to have multiple databases loaded at the same time for different reasons,
// but these databases may have conflicting IDs in them. In this case, a system to manage priority is required.
//
// This also handles authentication, but will outsource it to a master server if set.
#include "NoobWarrior/EmuDb/ContentImages.h"
#include <NoobWarrior/EmuDb/EmuDbManager.h>
#include <NoobWarrior/EmuDb/EmuDb.h>
#include <NoobWarrior/Config.h>
#include <NoobWarrior/NoobWarrior.h>

#include <vector>

using namespace NoobWarrior;

EmuDbManager::EmuDbManager(Core *core) :
    mCore(core)
{}

void EmuDbManager::MountDatabases() {
    int filePriority = 0;
    auto mounted = mCore->GetConfig()->GetKeyValue<nlohmann::json>("databases.mounted");
    if (!mounted.has_value())
        return;

    for (auto &fileNameElement : *mounted) {
        if (!fileNameElement.is_string()) continue;
        auto fileName = fileNameElement.get<std::string>();
        Mount(fileName, filePriority);
        filePriority++;
    }
}

void EmuDbManager::UnmountDatabases() {
    for (auto *db : mMountedDatabases) {
        Unmount(db);
        NOOBWARRIOR_FREE_PTR(db)
    }
}

SqlDb::Response EmuDbManager::CreateMasterDatabaseIfDoesntExist() {
    if (GetMasterDatabase() == nullptr) {
        Out("EmuDbManager", "Creating master database because it doesn't exist...");
        std::filesystem::path absolutePath = mCore->GetUserDataDir() / "databases" / "master.nwdb";
        auto *db = new EmuDb(absolutePath.string(), true);
        if (db->Fail()) {
            Out("EmuDbManager", "Failed to create master database");
            return SqlDb::Response::DatabaseFailed;
        }
        db->SetTitle("Master Database");
        db->SetDescription("This is the default database created by noobWarrior.\nThis will only be the primary database if it is the highest on the list.");
        bool res = Mount(db, 0);
        return SqlDb::Response::Success;
    }
    return SqlDb::Response::DidNothing;
}

SqlDb::FailReason EmuDbManager::Mount(const std::string &fileName, unsigned int priority) {
    std::filesystem::path absolutePath = mCore->GetUserDataDir() / "databases" / fileName;
    auto *database = new EmuDb(absolutePath.string(), true);
    if (database->Fail()) return database->GetFailReason();
    mMountedDatabases.insert(mMountedDatabases.begin() + priority, database);
    return database->GetFailReason();
}

bool EmuDbManager::Mount(EmuDb* database, unsigned int priority) {
    if (database->Fail()) return false;
    if (std::find(mMountedDatabases.begin(), mMountedDatabases.end(), database) != mMountedDatabases.end())
        return false;
    mMountedDatabases.insert(mMountedDatabases.begin() + priority, database);
    Out("EmuDbManager", "Mounted database \"{}\"", database->GetFileName());
    return true;
}

bool EmuDbManager::Unmount(EmuDb* database) {
    auto it = std::find(mMountedDatabases.begin(), mMountedDatabases.end(), database);
    if (it == mMountedDatabases.end())
        return false;
    mMountedDatabases.erase(it);
    Out("EmuDbManager", "Unmounted database \"{}\"", database->GetFileName());
    return true;
}

EmuDb *EmuDbManager::GetMasterDatabase() {
    return mMountedDatabases.size() > 0 ? mMountedDatabases.at(0) : nullptr;
}

std::vector<EmuDb*> EmuDbManager::GetMountedDatabases() {
    return mMountedDatabases;
}

bool EmuDbManager::GetUserFromToken(User *user, const std::string &token) {
    *user = User {};
    return true;
}
