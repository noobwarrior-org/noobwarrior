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
#include <NoobWarrior/EmuDb/EmuDbManager.h>
#include <NoobWarrior/EmuDb/EmuDb.h>
#include <NoobWarrior/Config.h>

#include <vector>

using namespace NoobWarrior;

SqlDb::Response EmuDbManager::AutocreateMasterDatabase() {
    if (GetMasterDatabase() != nullptr)
        return SqlDb::Response::Success;
    return SqlDb::Response::Success;
}

SqlDb::FailReason EmuDbManager::Mount(const std::filesystem::path &filePath, unsigned int priority) {
    auto *database = new EmuDb(filePath.string());
    if (database->Fail()) return database->GetFailReason();
    MountedDatabases.insert(MountedDatabases.begin() + priority, database);
    return database->GetFailReason();
}

void EmuDbManager::Mount(EmuDb *database, unsigned int priority) {
    MountedDatabases.insert(MountedDatabases.begin() + priority, database);
}

EmuDb *EmuDbManager::GetMasterDatabase() {
    return MountedDatabases.size() > 0 ? MountedDatabases.at(0) : nullptr;
}

bool EmuDbManager::GetUserFromToken(User *user, const std::string &token) {
    *user = User {};
    return true;
}
