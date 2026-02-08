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
// File: EmuDbProject.cpp
// Started by: Hattozo
// Started on: 2/2/2024
// Description:
#include "EmuDbProject.h"
#include "NoobWarrior/EmuDb/EmuDb.h"
#include "Sdk/Sdk.h"
#include <NoobWarrior/Macros.h>

using namespace NoobWarrior;

EmuDbProject::EmuDbProject(const std::string &path) : Project(),
    mDb(new EmuDb(path, false))
{
    if (mDb->IsMemory()) {
        // If we're making a new file, then fill in some defaults (like the author of the database) with the name of the
        // person running the program.
        if (mDb->GetAuthor().empty()) {
            QString name = qgetenv("USER");
            if (name.isEmpty())
                name = qgetenv("USERNAME");
            mDb->SetAuthor(name.toStdString());
        }

        // It got marked as dirty because we programmatically changed the author of the DB.
        // Unmark it so that you won't get the stupid "you forgot to save your changes" screen when closing this empty new file.
        mDb->UnmarkDirty();
    }

    // our own functions
    mOverviewWidget = new OverviewWidget(mDb);
    mTabWidget->setCurrentIndex(mTabWidget->addTab(mOverviewWidget, "Overview"));
}

EmuDbProject::~EmuDbProject() {
    mTabWidget->deleteLater();
    NOOBWARRIOR_FREE_PTR(mDb)
}

EmuDb* EmuDbProject::GetDb() {
    return mDb;
}

bool EmuDbProject::Fail() {
    return mDb->Fail();
}

std::string EmuDbProject::GetFailMsg() {
    return mDb->GetLastErrorMsg();
}

QString EmuDbProject::GetTitle() {
    return !mDb->IsMemory() ? QString::fromStdString(mDb->GetFileName()) : "Unsaved File";
}

QIcon EmuDbProject::GetIcon() {
    return QIcon(":/images/silk/database.png");
}

bool EmuDbProject::IsDirty() {
    return mDb->IsDirty();
}

bool EmuDbProject::Save() {
    SqlDb::Response res = mDb->WriteChangesToDisk();
    mLastSaveRes = res;
    return res == SqlDb::Response::Success;
}

std::string EmuDbProject::GetSaveFailMsg() {
    switch (mLastSaveRes) {
        default: return "Unknown failure: is this file read-only?"; break;
        case SqlDb::Response::Success: return "Didn't fail";
        case SqlDb::Response::Busy: return "The database seems to be busy.";
        case SqlDb::Response::Misuse: return "There was an internal error.";
    }
}

void EmuDbProject::OnShown() { }
void EmuDbProject::OnHidden() { }
